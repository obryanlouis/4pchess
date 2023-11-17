#include "command_line.h"

#include <chrono>
#include <exception>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "player.h"
#include "transposition_table.h"
#include "board.h"
#include "utils.h"


namespace chess {

constexpr char kEngineName[] = "4pChess 0.1";
constexpr char kAuthorName[] = "Louis O.";

using std::chrono::milliseconds;
using std::chrono::system_clock;
using std::chrono::time_point;
using std::chrono::duration_cast;

namespace {

std::string LowerCase(const std::string& s) {
  std::string lower;
  for (char c : s) {
    lower += std::tolower(c);
  }
  return lower;
}

std::string GetPVStr(const AlphaBetaPlayer& player) {
  std::string pv;
  const PVInfo* pv_info = &player.GetPVInfo();
  while (pv_info != nullptr) {
    auto best_move = pv_info->GetBestMove();
    if (best_move.has_value()) {
      if (!pv.empty()) {
        pv += " ";
      }
      pv += best_move->PrettyStr();
    }
    pv_info = pv_info->GetChild().get();
  }
  return pv;
}

}  // namespace

CommandLine::CommandLine() {
  player_options_.num_threads = 1;
}

void CommandLine::Run() {
  // Runs on main thread
  while (running_) {
    std::string line;
    std::getline(std::cin, line);
    std::vector<std::string> parts = SplitStrOnWhitespace(line);
    HandleCommand(line, parts);
  }
}

void CommandLine::EnableDebug(bool enable) {
  std::lock_guard lock(mutex_);
  debug_ = enable;
  if (player_ != nullptr) {
    player_->EnableDebug(enable);
  }
}

void CommandLine::SetBoard(std::shared_ptr<Board> board) {
  std::lock_guard lock(mutex_);
  board_ = board;
}

void CommandLine::StopEvaluation() {
  std::lock_guard lock(mutex_);
  if (thread_ != nullptr) {
    if (player_ != nullptr) {
      player_->SetCanceled(true);
    }
    thread_->join();
    thread_.reset();
    if (player_ != nullptr) {
      player_->SetCanceled(false);
    }
  }
}

void CommandLine::ResetPlayer() {
  std::lock_guard lock(mutex_);
  //PlayerOptions options;
  //options.enable_multithreading = n_threads_ > 1;
  //options.num_threads = n_threads_;
  player_ = std::make_shared<AlphaBetaPlayer>(player_options_);
}

void CommandLine::ResetBoard() {
  std::lock_guard lock(mutex_);
  board_ = Board::CreateStandardSetup();
}

void CommandLine::SetEvaluationOptions(const EvaluationOptions& options) {
  std::lock_guard lock(mutex_);
  options_ = options;
}

void CommandLine::StartEvaluation() {
  std::lock_guard lock(mutex_);
  thread_ = std::make_unique<std::thread>([this]() {
    int depth = 1;
    std::shared_ptr<Board> board;
    std::shared_ptr<AlphaBetaPlayer> player;
    EvaluationOptions options;
    {
      std::lock_guard lock(mutex_);
      if (board_ == nullptr || player_ == nullptr) {
        // Should never happen.
        SendInfoMessage("Haven't set up board -- can't evaluate.");
        return;
      }
      board = board_;
      player = player_;
      options = options_;
    }

    // if the game is over, print a string showing that
    auto game_result = board->GetGameResult();
    if (game_result != IN_PROGRESS) {
      switch (game_result) {
      case WIN_RY:
        SendInfoMessage("Game completed. RY won."); 
        break;
      case WIN_BG:
        SendInfoMessage("Game completed. BG won."); 
        break;
      case STALEMATE:
        SendInfoMessage("Game completed. Stalemate."); 
        break;
      default:
        break;
      }
      return;
    }

    auto start = system_clock::now();
    int num_eval_start = player->GetNumEvaluations();
    std::optional<Move> best_move;

    std::optional<time_point<system_clock>> deadline;
    if (options.movetime.has_value()) {
      deadline = start + milliseconds(options.movetime.value());
    }
    std::optional<milliseconds> time_limit;

    while (!player->IsCanceled()
           && (!options.depth.has_value() || depth <= options.depth.value())
           && (!deadline.has_value()
               || system_clock::now() < deadline.value())
           // sanity check: past depth 100 won't help
           && depth < 100) {
      if (deadline.has_value()) {
        time_limit = duration_cast<milliseconds>(
            deadline.value() - system_clock::now());
      }
      auto res = player->MakeMove(*board, time_limit, depth);

      if (res.has_value()) {
        auto duration_ms = duration_cast<milliseconds>(
            system_clock::now() - start);
        int num_evals = player->GetNumEvaluations() - num_eval_start;
        std::optional<int> nps;
        if (duration_ms.count() > 0) {
          nps = (int) (((float)num_evals) / (duration_ms.count() / 1000.0));
        }
        int score_centipawn = std::get<0>(res.value());
        if (board->GetTurn().GetTeam() == BLUE_GREEN) {
          score_centipawn = -score_centipawn;
        }
        std::string pv = GetPVStr(*player);

        std::cout
          << "info"
          << " depth " << depth
          << " time " << duration_ms.count()
          << " nodes " << num_evals
          << " pv " << pv
          << " score " << score_centipawn;
        if (nps.has_value()) {
          std::cout << " nps " << nps.value();
        }
        std::cout << std::endl;

        best_move = std::get<1>(res.value());
        if (std::abs(score_centipawn) == kMateValue) {
          break;
        }

      } else {
        break;
      }

      depth++;
    }

    if (best_move.has_value()) {
      std::cout << "bestmove " << best_move->PrettyStr() << std::endl;
      best_move = std::nullopt;
    }

  });
}

void CommandLine::MakePonderMove() {
  std::lock_guard lock(mutex_);
}

void CommandLine::HandleCommand(
    const std::string& line,
    const std::vector<std::string>& parts) {
  if (parts.empty()) {
    return;
  }
  const auto& command = parts[0];
  if (command == "uci") {
    std::cout << "id name " << kEngineName << std::endl;
    std::cout << "id author " << kAuthorName << std::endl;

    // Allowed options
    std::cout << "option name Hash type spin default 100"
      << std::endl; // size in MB
    std::cout << "option name UCI_ShowCurrLine type check default false"
      << std::endl;

    std::cout << "uciok" << std::endl;
  } else if (command == "debug") {
    if (parts.size() != 2) {
      SendInvalidCommandMessage(line);
      return;
    }
    if (parts[1] == "on") {
      EnableDebug(true);
    } else if (parts[1] == "off") {
      EnableDebug(false);
    } else {
      SendInvalidCommandMessage(line);
      return;
    }
  } else if (command == "isready") {
    std::cout << "readyok" << std::endl;
  } else if (command == "setoption") {
    if (parts.size() != 5) {
      SendInvalidCommandMessage(line);
      return;
    }
    std::string option_name = LowerCase(parts[2]);
    const auto& option_value = parts[4];
    if (option_name == "hash") {
      auto val = ParseInt(option_value);
      if (val.has_value()) {
        if (val.value() < 0) {
          SendInvalidCommandMessage(
              "Hash MB must be non-negative, given: " + option_value);
          return;
        }
        //hash_table_mb_ = val.value();
        player_options_.transposition_table_size = val.value() * 1000000 / sizeof(HashTableEntry);
      } else {
        SendInvalidCommandMessage("Can not parse int: " + option_value);
        return;
      }
    } else if (option_name == "uci_showcurrline") {
      if (option_value == "true") {
        show_current_line_ = true;
      } else if (option_value == "false") {
        show_current_line_ = false;
      } else {
        SendInvalidCommandMessage(
              "UCI_ShowCurrLine option value must be 'true' or "
              "'false', given: " + option_value);
        return;
      }
    } else if (option_name == "threads") {
      int n_threads = 1;
      try {
        n_threads = std::stoi(option_value);
      } catch (const std::exception& e) {
        SendInvalidCommandMessage(
            "Invalid value for threads: " + option_value);
        return;
      }
      if (n_threads < 0) {
        SendInvalidCommandMessage("Num threads must be positive");
      }
      player_options_.num_threads = n_threads;
      player_options_.enable_multithreading = n_threads > 1;
    } else {
      for (const auto name : {
          "piece_eval_pawn",
          "piece_eval_knight",
          "piece_eval_bishop",
          "piece_eval_rook",
          "piece_eval_queen"}) {
        if (name == option_name) {
          try {
            player_options_.piece_eval_pawn = std::stoi(option_value);
            return;
          } catch (const std::exception& e) {
            SendInvalidCommandMessage(
                "Invalid value for " + option_name + ": " + option_value);
            return;
          }
        }
      }

      SendInvalidCommandMessage("Unrecognized option: " + option_name);
      return;
    }
    StopEvaluation();
    ResetPlayer();

  } else if (command == "register") {
    // ignore
  } else if (command == "ucinewgame") {
    // stop evaluation, if any, and create a new player / board
    StopEvaluation();
    ResetPlayer();
    ResetBoard();
  } else if (command == "position") {

    if (parts.size() < 2) {
      SendInvalidCommandMessage(line);
      return;
    }

    size_t next_pos = 1;
    std::shared_ptr<Board> board;

    if (parts[1] == "fen") {
      if (parts.size() < 3) {
        SendInvalidCommandMessage(line);
      }
      const auto& fen = parts[2];
      board = ParseBoardFromFEN(fen);
      if (board == nullptr) {
        SendInfoMessage("Invalid FEN: " + fen);
        return;
      }
      next_pos += 2;
    } else {
      if (parts[1] == "startpos") {
        next_pos += 1;
      }
      board = Board::CreateStandardSetup();
    }

    if (parts.size() < next_pos + 1) {
      // Invalid, but we'll accept it
      StopEvaluation();
      ResetPlayer();
      SetBoard(std::move(board));
      return;
    }
    if (parts[next_pos] != "moves") {
      SendInvalidCommandMessage(line);
      return;
    }

    next_pos++;
    for (size_t i = next_pos; i < parts.size(); i++) {
      const auto& move_str = parts[i];
      std::optional<Move> move_or = ParseMove(*board, move_str);
      if (!move_or.has_value()) {
        SendInfoMessage("Invalid move '" + move_str + "'");
        return;
      }
      board->MakeMove(move_or.value());
    }

    StopEvaluation();
    ResetPlayer();
    SetBoard(std::move(board));

  } else if (command == "go") {
    if (board_ == nullptr) {
      // allow "go" without calling "ucinewgame"
      ResetPlayer();
      ResetBoard();
    }

    // parse all options and then execute a search

    size_t cmd_id = 1;
    EvaluationOptions options;

    // integer options
    std::unordered_map<std::string, std::optional<int>*>
      option_name_to_value;
    option_name_to_value["movetime"] = &options.movetime;
    option_name_to_value["rtime"] = &options.red_time;
    option_name_to_value["btime"] = &options.blue_time;
    option_name_to_value["ytime"] = &options.yellow_time;
    option_name_to_value["gtime"] = &options.green_time;
    option_name_to_value["rinc"] = &options.red_time;
    option_name_to_value["binc"] = &options.blue_time;
    option_name_to_value["yinc"] = &options.yellow_time;
    option_name_to_value["ginc"] = &options.green_time;
    option_name_to_value["moves_to_go"] = &options.moves_to_go;
    option_name_to_value["depth"] = &options.depth;
    option_name_to_value["nodes"] = &options.nodes;
    option_name_to_value["mate"] = &options.mate;

    while (cmd_id < parts.size()) {
      const auto& option_name = parts[cmd_id];

      if (option_name_to_value.find(option_name)
          != option_name_to_value.end()) {
        if (parts.size() < cmd_id + 2) {
          SendInvalidCommandMessage(line);
          return;
        }
        const auto& int_str = parts[cmd_id + 1];
        auto* value = option_name_to_value[option_name];
        *value = ParseInt(int_str);
        if (!value->has_value()) {
          SendInvalidCommandMessage("Can not parse integer: {}" + int_str);
        }
        cmd_id += 2;
      } else if (option_name == "searchmoves") {
        options.search_moves.clear();
        size_t move_id = cmd_id + 1;

        while (move_id < parts.size()) {
          auto move = ParseMove(*board_, parts[move_id]);
          if (move.has_value()) {
            options.search_moves.push_back(std::move(move.value()));
            move_id++;
          } else {
            break;
          }
        }

        cmd_id += 1 + options.search_moves.size();
      } else if (option_name == "ponder") { 
        options.ponder = true;
        cmd_id++;
      } else if (option_name == "infinite") { 
        options.infinite = true;
        cmd_id++;
      }

    }

    StopEvaluation();
    SetEvaluationOptions(options);
    StartEvaluation();

  } else if (command == "stop") {
    // cancel current search, if any
    StopEvaluation();
  } else if (command == "ponderhit") {
    // switch from pondering to normal move
    StopEvaluation();
    MakePonderMove();
    StartEvaluation();
  } else if (command == "quit") {
    // exit the program
    StopEvaluation();
    running_ = false;
  } else {
    SendInvalidCommandMessage(line);
  }
}


}  // namespace chess

