#include <mutex>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>

#include "../board.h"
#include "../player.h"

namespace {

constexpr int kNumThreads = 12;
constexpr int kNumSamples = 1'000'000;
constexpr int kDepth = 4;
constexpr float kRandomMoveRate = 0.05;
constexpr float kRandomFENRate = 0.0;// 0.2;
constexpr float kRandomNoNNUERate = 0.5;
constexpr int kMaxMovesPerGame = 300;

}  // namespace


namespace chess {


namespace {

void SaveBoard(
    std::fstream& fs_board,
    std::fstream& fs_score,
    std::fstream& fs_per_depth_score,
    const Board& board,
    const Player& turn,
    int score,
    const std::vector<int> per_depth_score) {
  for (int turn_addition = 0; turn_addition < 4; turn_addition++) {
    PlayerColor color = static_cast<PlayerColor>((turn.GetColor() + turn_addition) % 4);
    for (int row = 0; row < 14; row++) {
      for (int col = 0; col < 14; col++) {
        Piece piece = board.GetPiece(row, col);
        int id = 0; // for missing
        if (piece.Present() && piece.GetColor() == color) {
          id = 1 + (int)piece.GetPieceType();
        }
        fs_board << id;
        if (row == 13 && col == 13 && turn_addition == 3) {
          fs_board << std::endl;
        } else {
          fs_board << ",";
        }
      }
    }
  }
  fs_score << score << std::endl;
  for (size_t i = 0; i < per_depth_score.size(); i++) {
    fs_per_depth_score << per_depth_score[i];
    if (i < per_depth_score.size() - 1) {
      fs_per_depth_score << ",";
    } else {
      fs_per_depth_score << std::endl;
    }
  }
}

std::unique_ptr<std::fstream> OpenFile(const std::string& filepath) {
  auto fs = std::make_unique<std::fstream>();
  fs->open(filepath, std::fstream::out | std::fstream::app);
  if (fs->fail()) {
    std::cout << "Can't open file: " << filepath << std::endl;
    abort();
  }
  return fs;
}

// Random float in [0, 1]
float RandFloat() {
  return (float)rand() / RAND_MAX;
}

int RandInt(int max) {
  return rand() % std::max(max, 1);
}

void SaveGameResults(
    GameResult game_result,
    PlayerColor final_turn,
    int num_moves,
    std::fstream& fs_result) {
  PlayerColor turn = static_cast<PlayerColor>((final_turn - num_moves) % 4);
  for (int i = 0; i < num_moves; i++) {
    switch (game_result) {
    case WIN_RY:
      if (turn == RED || turn == YELLOW) {
        fs_result << 1 << std::endl;
      } else {
        fs_result << -1 << std::endl;
      }
      break;
    case WIN_BG:
      if (turn == RED || turn == YELLOW) {
        fs_result << -1 << std::endl;
      } else {
        fs_result << 1 << std::endl;
      }
      break;
    case STALEMATE:
      fs_result << 0 << std::endl;
      break;
    case IN_PROGRESS:
      fs_result << 0 << std::endl;  // also recorded as a draw
      break;
    }
    turn = static_cast<PlayerColor>((turn + 1) % 4);
  }
}

bool IsLegalLocation(int row, int col) {
  if (row < 0
      || row > 13
      || col < 0
      || col > 13
      || (row < 3 && (col < 3 || col > 10))
      || (row > 10 && (col < 3 || col > 10))) {
    return false;
  }
  return true;
}

BoardLocation GetCandidateLocation(PieceType piece_type, PlayerColor color) {
  if (piece_type != PAWN) {
    while (true) {
      int row = RandInt(14);
      int col = RandInt(14);
      if (IsLegalLocation(row, col)) {
        return BoardLocation(row, col);
      }
    }
  }

  int min_row;
  int max_row;
  int min_col;
  int max_col;
  switch (color) {
  case RED:
    min_row = 4;
    max_row = 12;
    min_col = 3;
    max_col = 10;
    break;
  case BLUE:
    min_row = 3;
    max_row = 10;
    min_col = 1;
    max_col = 9;
    break;
  case YELLOW:
    min_row = 1;
    max_row = 9;
    min_col = 3;
    max_col = 10;
    break;
  case GREEN:
    min_row = 3;
    max_row = 10;
    min_col = 4;
    max_col = 12;
    break;
  default:
    abort();  // never happens
    break;
  }

  int row = min_row + RandInt(max_row - min_row);
  int col = min_col + RandInt(max_col - min_col);
  return BoardLocation(row, col);
}

void AddPiece(std::unordered_map<BoardLocation, Piece>& location_to_piece,
              PieceType piece_type,
              PlayerColor color) {
  BoardLocation location;
  while (true) {
    location = GetCandidateLocation(piece_type, color);
    if (location_to_piece.find(location) == location_to_piece.end()) {
      break;
    }
  }

  location_to_piece[location] = Piece(color, piece_type);
}

std::shared_ptr<Board> CreateBoardFromRandomFEN() {
  std::unordered_map<BoardLocation, Piece> location_to_piece;
  for (int color = 0; color < 4; color++) {
    PlayerColor col = static_cast<PlayerColor>(color);
    AddPiece(location_to_piece, KING, col);
    AddPiece(location_to_piece, QUEEN, col);
    AddPiece(location_to_piece, ROOK, col);
    AddPiece(location_to_piece, ROOK, col);
    AddPiece(location_to_piece, BISHOP, col);
    AddPiece(location_to_piece, BISHOP, col);
    AddPiece(location_to_piece, KNIGHT, col);
    AddPiece(location_to_piece, KNIGHT, col);
    for (int i = 0; i < 8; i++) {
      AddPiece(location_to_piece, PAWN, col);
    }
  }

  PlayerColor color = static_cast<PlayerColor>(RandInt(4));
  Player turn(color);
  return std::make_shared<Board>(turn, std::move(location_to_piece));
}

std::shared_ptr<Board> CreateBoardFromRandomMoves(
    const std::string& nnue_weights_filepath,
    std::shared_ptr<NNUE> copy_weights_from) {
  auto board = Board::CreateStandardSetup();
  // sample several moves from a weak player
  PlayerOptions options;
  if (copy_weights_from != nullptr) {
    options.enable_nnue = true;
    options.nnue_weights_filepath = nnue_weights_filepath;
  }
  AlphaBetaPlayer player(options, copy_weights_from);

  int num_random_moves = RandInt(9);
  int time_limit_ms = 10 + RandInt(30);
  std::chrono::milliseconds time_limit(time_limit_ms);
  Move buffer[300];
  for (int i = 0; i < num_random_moves; i++) {
    if (RandFloat() < 1.0) {
      size_t n_moves = board->GetPseudoLegalMoves2(buffer, 300);
      int move_id = RandInt(n_moves);
      board->MakeMove(buffer[move_id]);
    } else {
      auto res = player.MakeMove(*board, time_limit);
      if (!res.has_value() // timeout
          || !std::get<1>(res.value()).has_value() // missing move
          ) {
        break; // should never really happen, but if so just break
      }
      const auto& move = std::get<1>(res.value()).value();
      board->MakeMove(move);
    }
  }

  return board;
}

std::shared_ptr<Board> CreateRandomStartPosition(
    const std::string& nnue_weights_filepath,
    std::shared_ptr<NNUE> copy_weights_from) {
  if (RandFloat() < kRandomFENRate) {
    return CreateBoardFromRandomFEN();
  }
  return CreateBoardFromRandomMoves(nnue_weights_filepath, copy_weights_from);
}

}  // namespace

class GenData {
 public:

  GenData(int depth, int num_threads, int num_samples,
      std::string nnue_weights_filepath, float nnue_search_rate)
    : depth_(depth), num_threads_(num_threads), num_samples_(num_samples),
      nnue_weights_filepath_(std::move(nnue_weights_filepath)),
      nnue_search_rate_(nnue_search_rate) {
    enable_nnue_ = !nnue_weights_filepath_.empty();
    if (enable_nnue_) {
      copy_weights_from_ = std::make_shared<NNUE>(nnue_weights_filepath_);
    }
  }

  void Run(const std::string& output_dir) {

    start_ = std::chrono::system_clock::now();

    std::vector<std::unique_ptr<std::thread>> threads;
    for (int i = 0; i < num_threads_; i++) {
      auto run_games = [this, i, &output_dir]() {
        CreateData(output_dir, i);
      };
      auto thread = std::make_unique<std::thread>(run_games);
      threads.push_back(std::move(thread));
    }
    for (int i = 0; i < num_threads_; i++) {
      threads[i]->join();
    }

    auto duration = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now() - start_);
    std::cout << "Duration (sec): " << duration.count() << std::endl;
  }

  void CreateData(std::string output_dir, int thread_id) {
    std::filesystem::path output(output_dir);
    std::string tid = std::to_string(thread_id);
    auto fs_board = OpenFile(output / ("board_" + tid + ".csv"));
    auto fs_score = OpenFile(output / ("score_" + tid + ".csv"));
    auto fs_per_depth_score = OpenFile(output / ("per_depth_score_" + tid + ".csv"));
    auto fs_result = OpenFile(output / ("game_result_" + tid + ".csv"));

    PlayerOptions options;
    options.enable_nnue = enable_nnue_;
    options.nnue_weights_filepath = nnue_weights_filepath_;
    PlayerOptions options_without_nnue;
    options.enable_nnue = false;
    std::shared_ptr<Board> board;
    Move buffer[300];

    while (positions_calculated_ < num_samples_) {
      AlphaBetaPlayer player(options, copy_weights_from_);
      AlphaBetaPlayer player_without_nnue(options_without_nnue);

      board = Board::CreateStandardSetup();
      //board = CreateRandomStartPosition(
      //    nnue_weights_filepath_, copy_weights_from_);
      if (copy_weights_from_ != nullptr) {
        board->SetNNUE(copy_weights_from_.get());
      }

      int num_moves = 0;
      // loop until game over
      while (true) {
        if (num_moves >= kMaxMovesPerGame) {
          IncrGameResults(IN_PROGRESS);
          SaveGameResults(
              IN_PROGRESS, board->GetTurn().GetColor(), num_moves, *fs_result);
          break;
        }
        GameResult game_result = board->GetGameResult();
        if (game_result != IN_PROGRESS) {
          IncrGameResults(game_result);
          SaveGameResults(
              game_result, board->GetTurn().GetColor(), num_moves, *fs_result);
          break;
        }
        std::vector<int> per_depth_score;
        per_depth_score.reserve(depth_ + 1);

        AlphaBetaPlayer* p =
          RandFloat() <= nnue_search_rate_ ? &player : &player_without_nnue;

        p->ResetMobilityScores(*board);
        int score = p->Evaluate(*board, true);
        per_depth_score.push_back(score);
        std::optional<Move> move;
        PlayerColor color = board->GetTurn().GetColor();
        for (int depth = 1; depth <= depth_; depth++) {
          auto res = p->MakeMove(*board, /*time_limit=*/std::nullopt, depth).value();
          score = std::get<0>(res);

          // Output scores w.r.t. the current perspective
          if (color == BLUE || color == GREEN) {
            score = -score;
          }

          per_depth_score.push_back(score);
          move = std::get<1>(res);
        }
        if (!move.has_value()) {
          // should never happen
          std::cout << "no move found" << std::endl;
          abort();
        }
        SaveBoard(*fs_board, *fs_score, *fs_per_depth_score, *board,
            board->GetTurn(), score, per_depth_score);
        IncrementStats();
        num_moves++;
        if (RandFloat() < kRandomMoveRate) {
          size_t n_moves = board->GetPseudoLegalMoves2(buffer, 300);
          int rand_move = RandInt(n_moves);
          board->MakeMove(buffer[rand_move]);
        } else {
          board->MakeMove(move.value());
        }
      }

    }
  }

  void IncrementStats() {
    std::lock_guard lock(mutex_);
    positions_calculated_++;
    if (positions_calculated_ % 1000 == 0) {
      auto duration = std::chrono::duration_cast<std::chrono::seconds>(
          std::chrono::system_clock::now() - start_);
      float pos_per_sec = (float)positions_calculated_ / duration.count();
      size_t total_games = games_ry_won + games_bg_won + games_stalemate + games_unfinished;
      float ry_win_pct = 100.0 * (float)games_ry_won / (float)total_games;
      float bg_win_pct = 100.0 * (float)games_bg_won / (float)total_games;
      float stalemate_pct = 100.0 * (float)games_stalemate / (float)total_games;
      float unfinished_pct = 100.0 * (float)games_unfinished / (float)total_games;
      std::cout
        << "Positions calculated: " << positions_calculated_
        << " Positions/sec: " << pos_per_sec
        << std::setprecision(4)
        << " #games: " << total_games
        << " RY-win: " << ry_win_pct
        << " BG-win: " << bg_win_pct
        << " Draw: " << stalemate_pct
        << " Unfin: " << unfinished_pct
        << std::endl;
    }
  }

  void IncrGameResults(GameResult game_result) {
    std::lock_guard lock(mutex_);
    switch (game_result) {
    case WIN_RY:
      games_ry_won++;
      break;
    case WIN_BG:
      games_bg_won++;
      break;
    case STALEMATE:
      games_stalemate++;
      break;
    case IN_PROGRESS:
      games_unfinished++;
      break;
    }
  }

 private:
  int depth_ = 0;
  int num_threads_ = 0;
  size_t num_samples_ = 0;
  std::mutex mutex_;
  std::atomic<size_t> positions_calculated_ = 0;
  std::chrono::time_point<std::chrono::system_clock> start_;
  bool enable_nnue_ = false;
  std::string nnue_weights_filepath_;
  std::shared_ptr<NNUE> copy_weights_from_;
  float nnue_search_rate_ = 0;

  size_t games_ry_won = 0;
  size_t games_bg_won = 0;
  size_t games_stalemate = 0;
  size_t games_unfinished = 0;
};

}  // namespace chess

namespace {

void PrintUsage() {
  std::cout << "Usage: <prog> /absolute/output_dir [depth] [num_threads]"
    << " [num_samples] [nnue_weights_filepath]"
    << std::endl;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cout << "Missing output_dir" << std::endl;
    PrintUsage();
    return 1;
  }
  std::string output_dir(argv[1]);

  int depth = kDepth;
  if (argc >= 3) {
    depth = std::atoi(argv[2]);
  }
  if (depth <= 0) {
    std::cout << "Invalid depth: " << std::string(argv[2]) << std::endl;
    PrintUsage();
    return 1;
  }

  int num_threads = kNumThreads;
  if (argc >= 4) {
    num_threads = std::atoi(argv[3]);
  }
  if (num_threads <= 0) {
    std::cout << "Invalid num_threads: " << std::string(argv[3]) << std::endl;
    PrintUsage();
    return 1;
  }

  int num_samples = kNumSamples;
  if (argc >= 5) {
    num_samples = std::atoi(argv[4]);
  }
  if (num_samples <= 0) {
    std::cout << "Invalid num_samples: " << std::string(argv[4]) << std::endl;
    PrintUsage();
    return 1;
  }

  std::string nnue_weights_filepath;
  if (argc >= 6) {
    nnue_weights_filepath = std::string(argv[5]);
  }

  float nnue_search_rate = 0.5;
  if (argc >= 7) {
    nnue_search_rate = std::atof(argv[6]);
  }

  chess::GenData gen_data(depth, num_threads, num_samples, nnue_weights_filepath,
      nnue_search_rate);
  gen_data.Run(output_dir);
  return 0;
}

