#include <chrono>
#include <unordered_map>
#include <vector>
#include <gtest/gtest.h>
#include "gmock/gmock.h"
#include <thread>

#include "board.h"
#include "player.h"

namespace chess {
namespace {

TEST(Speed, ThreadTest) {
  auto start = std::chrono::system_clock::now();

  std::vector<std::unique_ptr<std::thread>> threads;
  for (int i = 0; i < 12*20; i++) {
    threads.push_back(std::make_unique<std::thread>([i] {
      int j = 1;
      j = i + j;
    }));
  }
  for (const auto& thread : threads) {
    thread->join();
  }

  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now() - start);
  std::cout << "Duration (ms): " << duration.count() << std::endl;
}

TEST(Speed, BoardTest) {
  auto start = std::chrono::system_clock::now();
  auto board = Board::CreateStandardSetup();
  PlayerOptions options;
  AlphaBetaPlayer player(options);
  options.enable_transposition_table = true;
  options.enable_multithreading = true;
  options.num_threads = 1;
  player.EnableDebug(true);

  std::chrono::milliseconds time_limit(10000);
  //auto res = player.MakeMove(*board, time_limit);
  auto res = player.MakeMove(*board, std::nullopt, 18);

  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now() - start);
  std::cout << "Duration (ms): " << duration.count() << std::endl;
  int nps = (int) ((((float)player.GetNumEvaluations()) / duration.count())*1000.0);
  std::cout << "Nodes/sec: " << nps << std::endl;
  std::cout << "Nodes: " << player.GetNumEvaluations() << std::endl;
  if (options.enable_check_extensions) {
    std::cout << "#Check extensions: " << player.GetNumCheckExtensions() << std::endl;
  }
  if (options.enable_late_move_pruning) {
    std::cout << "#LM pruned: " << player.GetNumLateMovesPruned() << std::endl;
  }
  if (options.enable_transposition_table) {
    std::cout << "#Cache hits: " << player.GetNumCacheHits() << std::endl;
    float cache_hit_rate = (float)player.GetNumCacheHits() /
      (float)player.GetNumEvaluations();
    std::cout << "Cache hit rate: " << cache_hit_rate << std::endl;
  }
  if (options.enable_late_move_reduction) {
    std::cout << "#LMR searches: " << player.GetNumLmrSearches()
      << std::endl;
    std::cout << "#LMR re-searches: " << player.GetNumLmrResearches()
      << std::endl;
  }
  if (options.enable_null_move_pruning) {
    std::cout << "#Null moves tried: " << player.GetNumNullMovesTried()
      << std::endl;
    std::cout << "#Null moves pruned: " << player.GetNumNullMovesPruned()
      << std::endl;
  }
  int64_t lazy_eval = player.GetNumLazyEval();
  if (lazy_eval > 0) {
    std::cout << "#Lazy eval: " << lazy_eval << std::endl;
  }

  if (res.has_value() && std::get<1>(res.value()).has_value()) {
    const auto& val = res.value();
    std::cout << "Searched depth: " << std::get<2>(val) << std::endl;
    std::cout << "move: " << std::get<1>(val).value() << std::endl;
    std::cout << "evaluation: " << std::get<0>(val) << std::endl;
  } else {
    std::cout << "move missing" << std::endl;
  }

}

}  // namespace
}  // namespace chess

