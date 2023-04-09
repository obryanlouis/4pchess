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
#include "board.h"
#include "utils.h"


namespace chess {

constexpr char kEngineName[] = "4pChess 0.1";
constexpr char kAuthorName[] = "Louis O.";

namespace {

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
  if (best_move_.has_value()) {
    std::cout << "bestmove " << best_move_->PrettyStr() << std::endl;
    best_move_ = std::nullopt;
  }
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
  player_ = std::make_shared<AlphaBetaPlayer>();
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
    {
      std::lock_guard lock(mutex_);
      if (board_ == nullptr || player_ == nullptr) {
        // Should never happen.
        SendInfoMessage("Haven't set up board -- can't evaluate.");
        return;
      }
      board = board_;
      player = player_;
    }
    auto start = std::chrono::system_clock::now();
    int num_eval_start = player->GetNumEvaluations();

    while (!player->IsCanceled()) {
      bool canceled = false;
      {
        std::lock_guard lock(mutex_);
        canceled = player_->IsCanceled();
      }
      if (canceled) {
        break;
      }

      auto res = player->MakeMove(*board, /*time_limit=*/std::nullopt, depth);

      if (res.has_value()) {
        auto duration_ms = std::chrono::duration_cast<
          std::chrono::milliseconds>(std::chrono::system_clock::now() - start);
        int num_evals = player->GetNumEvaluations() - num_eval_start;
        std::optional<int> nps;
        if (duration_ms.count() > 0) {
          nps = (int) (((float)num_evals) / (duration_ms.count() / 1000.0));
        }
        int score_centipawn = std::get<0>(res.value());
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

        {
          std::lock_guard lock(mutex_);
          best_move_ = std::get<1>(res.value());
        }

      } else {
        break;
      }

      depth++;
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
    const auto& option_name = parts[2];
    const auto& option_value = parts[4];
    if (option_name == "Hash") {
      auto val = ParseInt(option_value);
      if (val.has_value()) {
        if (val.value() < 0) {
          SendInvalidCommandMessage(
              "Hash MB must be non-negative, given: " + option_value);
          return;
        }
        hash_table_mb_ = val.value();
      } else {
        SendInvalidCommandMessage("Can not parse int: " + option_value);
        return;
      }
    } else if (option_name == "UCI_ShowCurrLine") {
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
    }

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
      SendInfoMessage("Need to set up board first");
      return;
    }

    // parse all options and then execute a search

    size_t cmd_id = 1;
    EvaluationOptions options;

    // integer options
    std::unordered_map<std::string, std::optional<int>*>
      option_name_to_value;
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

      } else if (option_name == "ponder") { 
        options.ponder = true;
      } else if (option_name == "infinite") { 
        options.infinite = true;
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
    running_ = false;
  } else {
    SendInvalidCommandMessage(line);
  }
}


}  // namespace chess

