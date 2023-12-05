#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

#include "board.h"
#include "utils.h"
#include "player.h"

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"

ABSL_FLAG(std::string, fens_filepath, "",
    "FENs filepath. The program will sample positions from this file.");
ABSL_FLAG(int32_t, num_fens, 100, "Number of FENs to sample");
ABSL_FLAG(int32_t, move_ms, 250, "Move time in milliseconds");
ABSL_FLAG(int32_t, num_threads, 12, "Number of threads");

namespace chess {

namespace {

std::vector<std::string> ParseFENs(const std::string& fens_filepath) {
  std::ifstream infile(fens_filepath);
  std::string line;
  std::vector<std::string> fens;
  fens.reserve(10000);
  while (std::getline(infile, line)) {
    if (line.size() < 10) {
      continue;
    }
    fens.push_back(line);
  }
  return fens;
}

int SearchFEN(Board& board, int move_ms) {
  PlayerOptions options;
  options.num_threads = 1;
  AlphaBetaPlayer player(options);
  std::chrono::milliseconds time_limit(move_ms);
  auto res = player.MakeMove(board, time_limit);
  if (!res.has_value()) {
    std::cout << "No move value!" << std::endl;
    abort();
  }
  return std::get<2>(res.value());
}

class Runner {
 public:
  Runner() {
    std::string fens_filepath = absl::GetFlag(FLAGS_fens_filepath);
    if (fens_filepath.empty()
        || !std::filesystem::exists(fens_filepath)) {
      std::cout << "FENs filepath not found: " << fens_filepath << std::endl;
      abort();
    }
    fens_ = ParseFENs(fens_filepath);
    std::cout << "# FENs: " << fens_.size() << std::endl;
    move_ms_ = absl::GetFlag(FLAGS_move_ms);
    std::cout << "move ms: " << move_ms_ << std::endl;
    num_fens_ = absl::GetFlag(FLAGS_num_fens);
    std::cout << "num fens: " << num_fens_ << std::endl;
  }

  void Run() {
    std::vector<std::unique_ptr<std::thread>> threads;
    int num_threads = absl::GetFlag(FLAGS_num_threads);
    threads.reserve(num_threads);
    for (int i = 0; i < num_threads; i++) {
      threads.push_back(std::make_unique<std::thread>([this]() {
        SearchThread();
      }));
    }
    for (int i = 0; i < num_threads; i++) {
      threads[i]->join();
    }
  }

  void SearchThread() {
    while (true) {
      int fen_id = fen_id_.fetch_add(1);
      const std::string& fen = fens_[fen_id % fens_.size()];
      auto board = ParseBoardFromFEN(fen);
      if (board == nullptr) {
        continue;
      }
      int depth = SearchFEN(*board, move_ms_);
      {
        std::lock_guard<std::mutex> lock(mutex_);
        num_fens_processed_++;
        total_depth_ += depth;
        if (num_fens_processed_ >= num_fens_) {
          break;
        }
      }
      Report();
    }
  }

  void Report() {
    std::lock_guard<std::mutex> lock(mutex_);
    float avg_depth = (float)total_depth_ / (float)num_fens_processed_;
    std::cout << "avg depth: " << avg_depth << std::endl;
  }

 private:
  std::mutex mutex_;
  std::vector<std::string> fens_;
  int move_ms_ = 0;
  int num_fens_ = 0;
  int total_depth_ = 0;
  int num_fens_processed_ = 0;
  std::atomic<int> fen_id_ = 0;
};

}  // namespace


void RunDepthTest() {
  Runner runner;
  runner.Run();
}

}

int main(int argc, char* argv[]) {
  absl::ParseCommandLine(argc, argv);
  chess::RunDepthTest();
  return 0;
}

