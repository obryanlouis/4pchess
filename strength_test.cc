#include <functional>
#include <chrono>
#include <vector>
#include <iostream>
#include <mutex>
#include <thread>

#include "board.h"
#include "player.h"

namespace chess {

constexpr int kNumGames = 100;
constexpr int kMaxMovesPerGame = 100;
constexpr int kNumThreads = 10;
constexpr int kMoveTimeLimitMs = 15000;

class StrengthTest {
 public:
  StrengthTest() {
    if (kMoveTimeLimitMs <= 0) {
      move_time_limit_ = std::nullopt;
    } else {
      move_time_limit_ = std::chrono::milliseconds(kMoveTimeLimitMs);
    }

    player1_options_.enable_quiescence = false;
    player2_options_.enable_quiescence = true;
  }

  void Run() {
    auto start = std::chrono::system_clock::now();
    std::vector<std::unique_ptr<std::thread>> threads;
    for (int i = 0; i < kNumThreads; i++) {
      auto run_games = [this]() {
        RunGames();
      };
      auto thread = std::make_unique<std::thread>(run_games);
      threads.push_back(std::move(thread));
    }
    for (int i = 0; i < kNumThreads; i++) {
      threads[i]->join();
    }
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now() - start);
    std::cout << "Duration (sec): " << duration.count() << std::endl;
  }

  void RunGames() {
    while (true) {
      int game_id = StartGame();
      if (game_id >= kNumGames) {
        break;
      }

      float player2_score = 0;

      auto board = Board::CreateStandardSetup();
      bool player2_moves_first = game_id % 2 == 1;
      bool end_early = false;
      for (int move_id = 0; move_id < kMaxMovesPerGame; move_id++) {
        bool player1_moves = ((game_id + move_id) % 2 == 0);
        auto* player_options = player1_moves
          ? &player1_options_ : &player2_options_;
        AlphaBetaPlayer player(*player_options);

        auto res = player.MakeMove(*board, move_time_limit_);

        if (res.has_value()) {
          int search_depth = std::get<2>(res.value());
          if (player1_moves) {
            player1_num_searches_++;
            player1_total_search_depth_ += search_depth;
          } else {
            player2_num_searches_++;
            player2_total_search_depth_ += search_depth;
          }
        }

        GameResult game_result = board->GetGameResult();
        if (game_result != IN_PROGRESS) {
          if (game_result == STALEMATE) {
            player2_score += 0.5;
          } else if (player2_moves_first == (game_result == WIN_RY)) {
            player2_score += 1;
          } else {
          }
          end_early = true;
          break;
        }
        if (!res.has_value() || !std::get<1>(res.value()).has_value()) {
          if (!res.has_value()) {
            std::cout << "player lost due to timeout" << std::endl;
          } else {
            std::cout << "player lost due to missing move" << std::endl;
          }
          // player failed to move -- they lose
          if ((game_id % 2) == (move_id % 2)) {
            player2_score += 1;
          }
          end_early = true;
          break;
        }
        auto move = std::get<1>(res.value()).value();
        board->MakeMove(move);
      }
      if (!end_early) {
        int piece_eval = board->PieceEvaluation();
        if (piece_eval == 0) {
          player2_score += 0.5;
        } else {
          if ((piece_eval > 0) == (game_id % 2 == 1)) {
            player2_score += 1;
          }
        }
      }

      FinishGame(player2_score);
    }
  }

  int StartGame() {
    std::lock_guard lock(mutex_);
    return next_game_id_++;
  }

  void FinishGame(float player2_score) {
    std::lock_guard lock(mutex_);
    player2_score_ += player2_score;
    num_completed_games_++;

    float player2_win_rate = (float)player2_score_ / (float)num_completed_games_;
    float player1_avg_search_depth = (float)player1_total_search_depth_ / (float)player1_num_searches_;
    float player2_avg_search_depth = (float)player2_total_search_depth_ / (float)player2_num_searches_;
    std::cout
      << "Game: " << num_completed_games_
      << " Player 2 win rate: " << player2_win_rate
      << " Player 1 avg search depth: " << player1_avg_search_depth
      << " Player 2 avg search depth: " << player2_avg_search_depth
      << std::endl;
  }

 private:
  std::mutex mutex_;
  float player2_score_ = 0;
  int next_game_id_ = 0;
  int num_completed_games_ = 0;
  std::optional<std::chrono::milliseconds> move_time_limit_;
  PlayerOptions player1_options_;
  PlayerOptions player2_options_;
  int player1_num_searches_ = 0;
  int player1_total_search_depth_ = 0;
  int player2_num_searches_ = 0;
  int player2_total_search_depth_ = 0;
};

void RunStrengthTest() {
  StrengthTest test;
  test.Run();
}

}

int main(int argc, char* argv[]) {
  chess::RunStrengthTest();
  return 0;
}

