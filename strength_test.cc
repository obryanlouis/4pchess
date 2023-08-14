#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

#include "board.h"
#include "player.h"

namespace chess {

constexpr int kNumGames = 100;
constexpr int kMaxMovesPerGame = 200;
constexpr int kNumThreads = 12;
constexpr int kMoveTimeLimitMs = 5000;

namespace {

enum GameResultStatus {
  GAME_ENDED,  // game ended normally (win/loss/stalemate)
  MOVE_LIMIT,  // performed max # moves
  TIMEOUT,  // player failed to move in time
  MISSING_MOVE,  // player failed to return a move
  WINNING_EVAL,  // piece evaluation is too large
};

void SaveGame(
    std::string filepath,
    const std::vector<Move>& moves,
    bool player2_moves_first,
    float player2_score,
    GameResultStatus game_status) {
  std::fstream fs;
  fs.open(filepath, std::fstream::out);
  int first_player_id = player2_moves_first ? 2 : 1;
  fs << "[Player " << first_player_id << " moves first]" << std::endl;
  fs << "[Player 2 score: " << player2_score << "]" << std::endl;
  fs << "[Game result status: ";
  switch (game_status) {
  case GAME_ENDED:
    fs << "game ended normally (win/loss/stalemate)";
    break;
  case MOVE_LIMIT:
    fs << "move limit (" << kMaxMovesPerGame << " moves)";
    break;
  case TIMEOUT:
    fs << "player timeout";
    break;
  case MISSING_MOVE:
    fs << "player missing move";
    break;
  case WINNING_EVAL:
    fs << "winning eval";
    break;
  }
  fs << "]" << std::endl;
  for (int i = 0; i < (int)moves.size(); i++) {
    if (i % 4 == 0) {
      int move_id = 1 + i / 4;
      fs << move_id << "." << " ";
    }

    const auto& move = moves[i];
    fs << move.PrettyStr();

    if (i % 4 == 3) {
      fs << std::endl;
    } else {
      fs << " ";
    }
  }
}

}  // namespace

class StrengthTest {
 public:
  StrengthTest(std::string save_dir) {
    if (kMoveTimeLimitMs <= 0) {
      move_time_limit_ = std::nullopt;
    } else {
      move_time_limit_ = std::chrono::milliseconds(kMoveTimeLimitMs);
    }
    if (!save_dir.empty()) {
      save_dir_ = std::filesystem::path(save_dir);

      for (const auto & entry : std::filesystem::directory_iterator(save_dir)) {
        size_t pos = entry.path().filename().string().rfind(".pgn");
        if (pos == std::string::npos) {
          std::cout << "Directory is not empty an contains files "
                       "that are not *.pgn. Quitting!"
                    << std::endl;
          std::abort();
        }
      }

      std::filesystem::remove_all(save_dir_.value());
      std::filesystem::create_directory(save_dir_.value());
    }

    player1_options_.enable_piece_square_table = false;
    player2_options_.enable_piece_square_table = true;
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
      GameResultStatus game_status = GAME_ENDED;
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
          game_status = GAME_ENDED;
          break;
        }
        if (!res.has_value() || !std::get<1>(res.value()).has_value()) {
          if (!res.has_value()) {
            std::cout << "player lost due to timeout" << std::endl;
            game_status = TIMEOUT;
          } else {
            std::cout << "player lost due to missing move" << std::endl;
            game_status = MISSING_MOVE;
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
        int piece_eval = board->PieceEvaluation();
        if (std::abs(piece_eval) >= 30'000) {
          if ((piece_eval > 0) == player2_moves_first) {
            player2_score += 1;
          }
          end_early = true;
          game_status = WINNING_EVAL;
          break;
        }
      }
      if (board->NumMoves() == kMaxMovesPerGame) {
        game_status = MOVE_LIMIT;
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

      if (save_dir_.has_value()) {
        std::filesystem::path filepath = save_dir_.value() / ("game_" + std::to_string(game_id) + ".pgn");
        SaveGame(filepath, board->Moves(), player2_moves_first,
            player2_score, game_status);
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
  std::optional<std::filesystem::path> save_dir_;
};

void RunStrengthTest(std::string save_dir) {
  StrengthTest test(save_dir);
  test.Run();
}

}

int main(int argc, char* argv[]) {
  std::string save_dir;
  if (argc > 1) {
    save_dir = std::string(argv[1]);
  } else if (std::filesystem::exists("/tmp")) {
    save_dir = "/tmp/games";
  }
  if (!save_dir.empty()) {
    std::cout << "Will save games to dir: " << save_dir << std::endl;
  }
  chess::RunStrengthTest(save_dir);
  return 0;
}
