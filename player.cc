#include <tuple>
#include <algorithm>
#include <cmath>
#include <utility>
#include <cassert>
#include <functional>
#include <optional>
#include <iostream>
#include <tuple>
#include <stdexcept>
#include <cstring>

#include "board.h"
#include "player.h"

namespace chess {

constexpr int kMaxQuiescenceDepth = 12;
constexpr int kMaxQuiescenceMoves = 2;
constexpr int kMateValue = 100'00;  // mate value (centipawns)

AlphaBetaPlayer::AlphaBetaPlayer(std::optional<PlayerOptions> options) {
  if (options.has_value()) {
    options_ = options.value();
  }
  piece_evaluations_[PAWN] = 1;
  piece_evaluations_[KNIGHT] = 3;
  piece_evaluations_[BISHOP] = 4;
  piece_evaluations_[ROOK] = 5;
  piece_evaluations_[QUEEN] = 10;
  piece_evaluations_[KING] = 100;

  piece_move_order_scores_[PAWN] = 10;
  piece_move_order_scores_[KNIGHT] = 20;
  piece_move_order_scores_[BISHOP] = 30;
  piece_move_order_scores_[ROOK] = 40;
  piece_move_order_scores_[QUEEN] = 50;
  piece_move_order_scores_[KING] = 0;

//  if (options_.enable_transposition_table) {
//    hash_table_ = (HashTableEntry*) calloc(
//        options_.transposition_table_size, sizeof(HashTableEntry));
//    assert(
//        (hash_table_ != nullptr) && 
//        "Can't create transposition table. Try using a smaller size.");
//  }

  for (int i = 1; i < kMaxMoves; i++) {
    reductions_[i] = int(10.0 * std::log(i));
  }

  if (options_.enable_history_heuristic) {
    ResetHistoryHeuristic();
  }

  if (options_.enable_history_leaf_pruning) {
    ResetHistoryCounters();
  }
}

int AlphaBetaPlayer::Reduction(int depth_left, int move_number) const {
  return (reductions_[depth_left] * reductions_[move_number] + 1500) / 1000;
}


//// https://www.chessprogramming.org/Quiescence_Search
//std::optional<int> AlphaBetaPlayer::QuiescenceSearch(
//    Board& board,
//    int depth_left,
//    int alpha,
//    int beta,
//    bool maximizing_player,
//    int moves_since_last_active,
//    const std::optional<
//        std::chrono::time_point<std::chrono::system_clock>>& deadline) {
//  if (canceled_.load() || (deadline.has_value()
//        && std::chrono::system_clock::now() >= deadline.value())) {
//    // hit deadline
//    return std::nullopt;
//  }
//
//  std::string debug_prefix(kMaxQuiescenceDepth - depth_left, ' ');
//
//  std::cout
//    << debug_prefix
//    << "QSearch"
//    << " last move: " << board.GetLastMove()
//    << " player: " << static_cast<int>(board.GetTurn().GetColor())
//    << " depth_left: " << depth_left
//    << " alpha: " << alpha
//    << " beta: " << beta
//    << " maximizing_player: " << maximizing_player
//    << " msla: " << moves_since_last_active
//    << std::endl;
//
//  int eval = Evaluate(board, maximizing_player);
//  if (depth_left <= 0 || moves_since_last_active >= 3) {
//    std::cout << debug_prefix << "QSearch res: " << eval << std::endl;
//    return eval;
//  }
//
//  std::optional<int> value_or;
//  Player player = board.GetTurn();
//  bool checked = board.IsKingInCheck(player);
//
//  if (!checked) {
//    // first try a null move -- use this as stand_pat
//    Player next_player = GetNextPlayer(player);
//    board.SetPlayer(next_player);
//
//    value_or = QuiescenceSearch(
//        board, depth_left - 1, -beta, -alpha, !maximizing_player,
//        moves_since_last_active + 1, deadline);
//    board.SetPlayer(player);
//
//    if (!value_or.has_value()) {
//      return std::nullopt;
//    }
//
//    int stand_pat = -value_or.value();
//    if (stand_pat >= beta) {
//      return beta;
//    }
//    if (alpha < stand_pat) {
//      alpha = stand_pat;
//    }
//  }
//
//  std::vector<Move> pseudo_legal_moves = MoveOrder(board, maximizing_player);
//
//  int turn_i = static_cast<int>(player.GetColor());
//  int curr_mob_score = player_mobility_scores_[turn_i];
//
//  int num_moves_examined = 0;
//  bool has_legal_move = false;
//
//  for (int i = pseudo_legal_moves.size() - 1;
//      i >= 0; // && num_moves_examined < kMaxQuiescenceMoves;
//      i--) {
//    const auto& move = pseudo_legal_moves[i];
//    bool delivers_check = board.DeliversCheck(move);
//    board.MakeMove(move);
//
//    if (board.CheckWasLastMoveKingCapture() != IN_PROGRESS) {
//      board.UndoMove();
//      if (options_.enable_mobility_evaluation) {
//        player_mobility_scores_[turn_i] = curr_mob_score; // reset
//      }
//
////      return beta;
////    std::cout << debug_prefix << "QSearch res: " << "inf" << std::endl;
//      return kMateValue;
//    }
//
//    if (board.IsKingInCheck(player)) { // Illegal move
//      board.UndoMove();
//
//      continue;
//    }
//
//    has_legal_move = true;
//
//    bool active = delivers_check || move.IsCapture();
//    // skip quiet moves after kMaxQuiescenceMoves
//    bool skip = num_moves_examined >= kMaxQuiescenceMoves && !active;
//
//    if (skip) {
//      board.UndoMove();
//      continue;
//    }
//
//    if (options_.enable_mobility_evaluation) {
//      int player_mobility_score = board.MobilityEvaluation(player);
//      player_mobility_scores_[turn_i] = player_mobility_score;
//    }
//
//    int msla = moves_since_last_active + 1;
//    if (active) {
//      std::cout
//        << debug_prefix
//        << "active -- delivers_check: " << delivers_check
//        << " is_capture: " << move.IsCapture()
//        << " move: " << move
//        << std::endl;
//      // active move
//      msla = 0;
//    }
//
//    value_or = QuiescenceSearch(
//        board, depth_left - 1, -beta, -alpha, !maximizing_player, msla,
//        deadline);
//
//    board.UndoMove();
//
//    num_moves_examined++;
//
//    if (!value_or.has_value()) {
//      return std::nullopt; // timeout
//    }
//
//    int score = -value_or.value();
//    if (score >= beta) {
//      if (options_.enable_mobility_evaluation) {
//        player_mobility_scores_[turn_i] = curr_mob_score; // reset
//      }
//    std::cout << debug_prefix << "QSearch res: " << beta << std::endl;
//
//      return beta;  // cutoff
//    }
//    if (score > alpha) {
//      alpha = score;
//    }
//  }
//
//  if (options_.enable_mobility_evaluation) {
//    player_mobility_scores_[turn_i] = curr_mob_score; // reset
//  }
//
//  if (!has_legal_move) {
//    // no legal moves
//    if (!checked) {
//    std::cout << debug_prefix << "QSearch res: " << 0 << std::endl;
//      return 0;  // stalemate
//    }
//
////    return alpha;
//    std::cout << debug_prefix << "QSearch res: " << "-inf" << std::endl;
//    return -kMateValue;  // checkmate
//  }
//
//  if (num_moves_examined == 0) {
//    return eval;
//  }
//
//    std::cout << debug_prefix << "QSearch res: " << alpha << std::endl;
//  return alpha;
//}


// Alpha-beta search with nega-max framework.
// https://www.chessprogramming.org/Alpha-Beta
// Returns (nega-max value, best move) pair.
// The best move is nullopt if the game is over.
// If the function returns std::nullopt, then it hit the deadline
// before finishing search and the results should not be used.
std::optional<std::tuple<int, std::optional<Move>>> AlphaBetaPlayer::Search(
    Board& board,
    int depth,
    int depth_left,
    int alpha,
    int beta,
    bool maximizing_player,
    int expanded,
    const std::optional<
        std::chrono::time_point<std::chrono::system_clock>>& deadline,
    PVInfo& pvinfo,
    int null_moves) {
  if (canceled_.load() || (deadline.has_value()
        && std::chrono::system_clock::now() >= deadline.value())) {
    return std::nullopt;
  }
  bool root_node = depth == 1;

//  if (options_.enable_transposition_table) { // return hash table value if any
//    int64_t hash = board.HashKey();
//    size_t n = hash % options_.transposition_table_size;
//    const HashTableEntry& entry = hash_table_[n];
//    if (entry.key == hash) { // valid entry
//      if (entry.depth >= depth_left) {
//        num_cache_hits_++;
//        if (entry.bound == EXACT) {
//          return std::make_tuple(entry.score, entry.move);
//        }
//
//        if (maximizing_player) {
//          if (entry.bound == LOWER_BOUND) {
//            alpha = std::max(alpha, entry.score);
//          } else {
//            beta = std::min(beta, entry.score);
//          }
//        } else {
//          if (entry.bound == UPPER_BOUND) {
//            alpha = std::max(alpha, -entry.score);
//          } else {
//            beta = std::min(beta, -entry.score);
//          }
//        }
//        if (beta <= alpha) {
//          return std::make_tuple(entry.score, entry.move);
//        }
//
//      }
//    }
//  }

  num_evaluations_++;
  int eval = Evaluate(board, maximizing_player);

  if (depth_left <= 0) {
//    if (options_.enable_quiescence) {
//
//      auto value_or = QuiescenceSearch(
//          board, kMaxQuiescenceDepth, alpha, beta, maximizing_player, 0, deadline);
//      if (!value_or.has_value()) {
//        return std::nullopt; // timeout
//      }
//      return std::make_tuple(value_or.value(), std::nullopt);
//    }

//    if (options_.enable_static_exchange) {
//      if (board.LastMoveWasCapture()) {
//        const Move& move = board.GetLastMove();
//        board.UndoMove();
//        eval -= StaticExchangeEvaluation(board, move); // sub since move undone
//        board.MakeMove(move);
//      }
//    }

    return std::make_tuple(eval, std::nullopt);
  }

  Player player = board.GetTurn();
  std::optional<Move> pv_move = pvinfo.GetBestMove();
  bool is_pv_node = pv_move.has_value();

//  // futility pruning
//  if (options_.enable_futility_pruning
//      && !is_pv_node // not a pv node
//      && depth > 1 // not root
//      && depth < 9 // important for mate finding
//      && eval >= beta + 150 * depth_left
//      ) {
//    num_futility_moves_pruned_++;
//    return std::make_pair(beta, std::nullopt);
//  }

  // null move pruning
  if (options_.enable_null_move_pruning
      && depth > 1  // not root
      && !is_pv_node // not a pv node
      && null_moves == 0 // last move wasn't null
      && !board.IsKingInCheck(player) // not in check
      && eval >= beta
      ) {
    num_null_moves_tried_++;
    board.MakeNullMove();

    // try the null move with possibly reduced depth
    PVInfo null_pvinfo;
    int r = std::min(depth_left / 3 + 1, depth_left - 1);
    auto value_and_move_or = Search(
        board, depth + 1, depth_left - r - 1,
        -beta, -beta + 1, !maximizing_player, expanded, deadline, null_pvinfo,
        null_moves + 1);

    board.UndoNullMove();

    // if it failed high, skip this move
    if (value_and_move_or.has_value()
        && -std::get<0>(value_and_move_or.value()) >= beta) {
      num_null_moves_pruned_++;
      return std::make_tuple(beta, std::nullopt);
    }
  }

  std::vector<Move> pseudo_legal_moves;
  pseudo_legal_moves = MoveOrder(board, maximizing_player, pv_move);

//  if (pv_move.has_value()) {
//    pseudo_legal_moves.push_back(pv_move.value());
//  }

  //int value = -kMateValue;
  std::optional<Move> best_move;
  int player_color = static_cast<int>(player.GetColor());
  int curr_mob_score = player_mobility_scores_[player_color];

//  if (options_.enable_extensions) {
//    // mate threat extension
//    if (null_moves == 0
//        && depth_left <= 1
//        && expanded <= 0) {
//      // check if the current player would be mated after a null move
//      // (blocks checkmates)
//      int mate_beta = kMateValue/2;
//      board.MakeNullMove();
//      PVInfo null_pvinfo;
//      auto zw_value_or = Search(
//          board, depth + 1, 4, mate_beta - 1, mate_beta,
//          !maximizing_player, expanded+4, deadline, null_pvinfo,
//          null_moves + 1);
//      board.UndoNullMove();
//      if (zw_value_or.has_value()
//          && mate_beta < std::get<0>(zw_value_or.value())) { // extend
//        extension = std::max(extension, 4);
//      }
//    }
//  }

  bool b_search_pv = true;
  std::optional<std::tuple<int, std::optional<Move>>> value_and_move_or;

  bool has_legal_moves = false;
  int move_count = 0;
  for (int i = pseudo_legal_moves.size() - 1; i >= 0; i--) {
    const auto& move = pseudo_legal_moves[i];
//    const auto* piece = board.GetPiece(move.From());

    bool delivers_check = false;
    bool lmr_cond1 =
      options_.enable_late_move_reduction
      && depth_left > 1
      && !is_pv_node
      && move_count > 2
      && !move.IsCapture()
      ;
    if (lmr_cond1) {
      // this has to be called before the move is made
      delivers_check = board.DeliversCheck(move);
    }

    board.MakeMove(move);

    if (board.CheckWasLastMoveKingCapture() != IN_PROGRESS) {
      board.UndoMove();
      alpha = beta; // fail hard
      //value = kMateValue;
      best_move = move;
      pvinfo.SetBestMove(move);
      break;
    }

    if (board.IsKingInCheck(player)) { // invalid move
      board.UndoMove();

      continue;
    }

    if (options_.enable_mobility_evaluation) {
      int player_mobility_score = board.MobilityEvaluation(player);
      player_mobility_scores_[player_color] = player_mobility_score;
    }

    has_legal_moves = true;
    bool is_pv_move = pv_move.has_value() && pv_move.value() == move;

    std::shared_ptr<PVInfo> child_pvinfo;
    if (is_pv_move && pvinfo.GetChild() != nullptr) {
      child_pvinfo = pvinfo.GetChild();
    } else {
      child_pvinfo = std::make_shared<PVInfo>();
    }

//    // history leaf pruning
//    if (options_.enable_history_leaf_pruning && allow_pruning) {
//      constexpr int kLeafThreshold = 500;
//      constexpr int kHistoryThreshold = kLeafThreshold * 2;
//      int from_row = move.From().GetRow();
//      int from_col = move.From().GetCol();
//      int to_row = move.To().GetRow();
//      int to_col = move.To().GetCol();
//      int played_nb = history_counter_[player_color][from_row][from_col][to_row][to_col];
//      if (played_nb >= 5) {
//        int history_score = history_heuristic_[from_row][from_col][to_row][to_col];
//        if (history_score < kHistoryThreshold) {
//          if (history_score < kLeafThreshold) {
//            board.UndoMove();
//            if (options_.enable_mobility_evaluation) { // reset
//              player_mobility_scores_[player_color] = curr_mob_score;
//            }
//
//            continue;  // history leaf pruning
//          }
//          r++;
//        }
//      }
//    }

    int extension = 0;
    if (options_.enable_extensions) {
//      // works, but is too expensive
//      // mate threat extension
//      if (null_moves == 0
//          && move.IsCapture()
//          && depth_left <= 1
//          && expanded <= 0
//          && extension <= 0) {
//        // check if the other team would be mated after a null move
//        // (finds checkmates)
//        int mate_beta = kMateValue/2;
//        board.MakeNullMove();  // next player makes null move
//        PVInfo null_pvinfo;
//        auto zw_value_or = Search(
//            board, depth + 1, 2, mate_beta - 1, mate_beta,
//            maximizing_player, expanded+2, deadline, null_pvinfo,
//            null_moves + 1);
//        board.UndoNullMove();
//        if (zw_value_or.has_value()
//            && mate_beta < std::get<0>(zw_value_or.value())) { // extend
//          extension = std::max(extension, 3);
//        }
//      }

    }

    bool lmr = lmr_cond1
        && !delivers_check
        && !board.IsKingInCheck(player.GetTeam());

    if (lmr) {
      num_lmr_searches_++;

      int r = Reduction(depth_left, move_count + 1);
      r = std::clamp(r, 0, depth_left - 2);
      value_and_move_or = Search(
          board, depth + 1, depth_left - 1 - r,
          -alpha-1, -alpha, !maximizing_player, expanded + extension,
          deadline, *child_pvinfo, null_moves);
      if (value_and_move_or.has_value()) {
        int score = -std::get<0>(value_and_move_or.value());
        if (score > alpha) {  // re-search
          num_lmr_researches_++;
          value_and_move_or = Search(
              board, depth + 1, depth_left - 1,
              -beta, -alpha, !maximizing_player, expanded + extension,
              deadline, *child_pvinfo, null_moves);
        }
      }

    } else {

      // pvs search
      if (!options_.pvs || (b_search_pv && is_pv_node)) {
        value_and_move_or = Search(
            board, depth + 1, depth_left - 1,
            -beta, -alpha, !maximizing_player, expanded + extension,
            deadline, *child_pvinfo, null_moves);

      } else {
        value_and_move_or = Search(
            board, depth + 1, depth_left - 1,
            -alpha-1, -alpha, !maximizing_player, expanded + extension,
            deadline, *child_pvinfo, null_moves);
        if (value_and_move_or.has_value()) {
          int score = -std::get<0>(value_and_move_or.value());
          if (score > alpha && (score < beta || root_node)) {  // re-search
            value_and_move_or = Search(
                board, depth + 1, depth_left - 1,
                -beta, -alpha, !maximizing_player, expanded + extension,
                deadline, *child_pvinfo, null_moves);
          }
        }

      }

    }

//    if (lmr
//        && value_and_move_or.has_value()
//        && std::get<0>(value_and_move_or.value()) > alpha) {
//      // Did not fail low -- redo search
//      value_and_move_or = Search(
//          board, depth + 1, depth_left - 1, -beta, -alpha, !maximizing_player,
//          expanded, deadline, *child_pvinfo);
//    }

    board.UndoMove();
    move_count++;

    if (options_.enable_mobility_evaluation) { // reset
      player_mobility_scores_[player_color] = curr_mob_score;
    }

    if (!value_and_move_or.has_value()) {
      return std::nullopt; // timeout
    }
    int score = -std::get<0>(value_and_move_or.value());

//    if (!best_move.has_value()) {
//      best_move = move;
//      // If PV move changed, update it
//      if (!pv_move.has_value() || pv_move.value() != move) {
//        pvinfo.SetBestMove(move);
//        pvinfo.SetChild(child_pvinfo);
//        pv_move = best_move;
//      }
//    }

    if (score >= beta) {
      alpha = beta;
      best_move = move;
      pvinfo.SetChild(child_pvinfo);
      pvinfo.SetBestMove(move);

      if (!move.IsCapture()) {
        const auto& from = move.From();
        const auto& to = move.To();
        if (options_.enable_history_heuristic) {
          history_heuristic_[from.GetRow()][from.GetCol()]
            [to.GetRow()][to.GetCol()] += (1 << depth_left);
        }
//        if (options_.enable_history_leaf_pruning) {
//          history_counter_[player_color][from.GetRow()][from.GetCol()]
//            [to.GetRow()][to.GetCol()]++;
//        }
      }

      break; // cutoff
    }
    if (score > alpha) {
      alpha = score;
      best_move = move;
      pvinfo.SetChild(child_pvinfo);
      pvinfo.SetBestMove(move);
      b_search_pv = false;
    }
    if (!best_move.has_value()) {
      best_move = move;
    }
  }

//  if (options_.enable_transposition_table) {
//    int64_t hash = board.HashKey();
//    size_t n = hash % options_.transposition_table_size;
//    HashTableEntry& entry = hash_table_[n];
//    if (entry.key != hash
//        || depth_left > entry.depth) {
//      entry.key = hash;
//      entry.depth = depth_left;
//      entry.move = best_move;
//      entry.score = maximizing_player ? alpha : -alpha;
//      if (value < alpha) {
//        entry.bound = maximizing_player ? UPPER_BOUND : LOWER_BOUND;
//      } else if (beta < value) {
//        entry.bound = maximizing_player ? LOWER_BOUND : UPPER_BOUND;
//      } else {
//        entry.bound = EXACT;
//      }
//    }
//  }

  if (!has_legal_moves) {
    if (!board.IsKingInCheck(board.GetTurn())) {
      // stalemate
      return std::make_tuple(std::min(beta, std::max(alpha, 0)), std::nullopt);
    }
    // checkmate
    return std::make_tuple(
        std::min(beta, std::max(alpha, -kMateValue)), std::nullopt);
  }

  return std::make_tuple(alpha, best_move);
}

int AlphaBetaPlayer::Evaluate(Board& board, bool maximizing_player) {
  int eval;
  GameResult game_result = board.CheckWasLastMoveKingCapture();
  if (game_result != IN_PROGRESS) { // game is over
    if (game_result == WIN_RY) {
      eval = kMateValue;
    } else if (game_result == WIN_BG) {
      eval = -kMateValue;
    } else {
      eval = 0; // stalemate
    }
  } else {
    eval = board.PieceEvaluation();
    if (options_.enable_mobility_evaluation) {
      for (int i = 0; i < 4; i++) {
        eval += player_mobility_scores_[i];
      }
    }
  }
  return maximizing_player ? eval : -eval;
}

void AlphaBetaPlayer::ResetHistoryHeuristic() {
  std::memset(history_heuristic_, 0, (14*14*14*14) * sizeof(int) / sizeof(char));
}

void AlphaBetaPlayer::ResetHistoryCounters() {
  std::memset(history_counter_, 0, (4*14*14*14*14) * sizeof(int) / sizeof(char));
}

std::optional<std::tuple<int, std::optional<Move>>> AlphaBetaPlayer::MTDF(
    Board& board,
    const std::optional<std::chrono::time_point<std::chrono::system_clock>>& deadline,
    int depth,
    int f) {
  int g = f;
  int upper_bound = kMateValue;
  int lower_bound = -kMateValue;
  bool maximizing_player = board.TeamToPlay() == RED_YELLOW;

  std::optional<std::tuple<int, std::optional<Move>>> move_and_value;
  std::optional<Move> best_move;
  while (lower_bound < upper_bound && (!deadline.has_value()
        || std::chrono::system_clock::now() < deadline.value())) {
    int b = std::max(g, lower_bound + 1);
    move_and_value = Search(
        board, 1, depth, b - 1, b, maximizing_player, 0,
        deadline, pv_info_);
    if (!move_and_value.has_value()) {
      return std::nullopt;
    }
    g = std::get<0>(move_and_value.value());
    if (g < b) {
      upper_bound = g;
    } else {
      lower_bound = g;
    }
  }
  return move_and_value;
}

void AlphaBetaPlayer::ResetMobilityScores(Board& board) {
  // reset pseudo-mobility scores
  if (options_.enable_mobility_evaluation) {
    for (int i = 0; i < 4; i++) {
      Player player(static_cast<PlayerColor>(i));
      player_mobility_scores_[i] = board.MobilityEvaluation(player);
    }
  }
}

std::optional<std::tuple<int, std::optional<Move>, int>> AlphaBetaPlayer::MakeMove(
    Board& board,
    std::optional<std::chrono::milliseconds> time_limit,
    int max_depth) {
  // Use Alpha-Beta search with iterative deepening

  if (options_.max_search_depth.has_value()) {
    max_depth = std::min(max_depth, options_.max_search_depth.value());
  }

  if (options_.enable_history_heuristic) {
    ResetHistoryHeuristic();
  }

  if (options_.enable_history_leaf_pruning) {
    ResetHistoryCounters();
  }

  std::optional<std::chrono::time_point<std::chrono::system_clock>> deadline;
  auto start = std::chrono::system_clock::now();
  if (time_limit.has_value()) {
    deadline = start + time_limit.value();
  }

  ResetMobilityScores(board);

  int next_depth = 1 + pv_info_.GetDepth();
  std::optional<std::tuple<int, std::optional<Move>>> res;
  int alpha = -kMateValue;
  int beta = kMateValue;
  bool maximizing_player = board.TeamToPlay() == RED_YELLOW;
  int searched_depth = 0;

  while (next_depth <= max_depth) {
    std::optional<std::tuple<int, std::optional<Move>>> move_and_value;

    if (options_.enable_mtdf) {
      int f = 0;
      if (res.has_value()) {
        f = std::get<0>(res.value());
      }
      move_and_value = MTDF(board, deadline, next_depth, f);
    } else {
      move_and_value = Search(
          board, 1, next_depth, alpha, beta, maximizing_player, 0,
          deadline, pv_info_);
    }

    if (!move_and_value.has_value()) { // Hit deadline
      break;
    }
    res = move_and_value;
    searched_depth = next_depth;
    next_depth++;
    int evaluation = std::get<0>(move_and_value.value());
    if (std::abs(evaluation) == kMateValue) {
      break;  // Proven win/loss
    }
  }

  if (res.has_value()) {
    int eval = std::get<0>(res.value());
    if (!maximizing_player) {
      eval = -eval;
    }
    return std::make_tuple(eval, std::get<1>(res.value()), searched_depth);
  }

  return std::nullopt;
}

std::vector<Move> AlphaBetaPlayer::MoveOrder(
    Board& board,
    bool maximizing_player,
    const std::optional<Move>& pvmove) {
  // Some heuristics on move ordering:
  // https://www.chessprogramming.org/Move_Ordering
  //
  // 1) Principal variation
  // 2) Hash moves
  // 3) Internal iterative deepening
  // 4) Recapturing pieces with the least valuable attacker
  // 5) Static exchange evaluation
  // 6) Killer heuristic
  // 7) History heuristic
  // 8) Relative history heuristic

  // Possible others:
  // * Checks
  // * Mate killers

  // For now, we'll skip all these heuristics and just go through the moves
  // randomly.

  // From experience:
  // 1. Checks
  // 2. Captures (higher value captures first)
  // 3. Higher-value piece moves

  if (!options_.enable_move_order) {
    return board.GetPseudoLegalMoves();
  }

  auto moves = board.GetPseudoLegalMoves();

  // reorder the moves
  std::vector<std::tuple<size_t, int>> pos_and_score;
  pos_and_score.reserve(moves.size());
  for (size_t i = 0; i < moves.size(); i++) {
    const auto& move = moves[i];
    int score = 0;

    if (pvmove.has_value() && move == pvmove.value()) {
      score += 1000'00;
    }

    if (options_.enable_move_order_checks) {
      if (board.DeliversCheck(move)) {
        score += 10'00;
      }
    }

    const auto* capture = move.GetStandardCapture();
    const auto* piece = board.GetPiece(move.From());

    if (capture != nullptr) {
      score += piece_evaluations_[static_cast<int>(capture->GetPieceType())];
      score -= piece_evaluations_[static_cast<int>(piece->GetPieceType())];
      score += 200;
    }

    score += piece_move_order_scores_[static_cast<int>(piece->GetPieceType())];

    if (options_.enable_history_heuristic && !move.IsCapture()) {
      const auto& from = move.From();
      const auto& to = move.To();
      score += history_heuristic_[from.GetRow()][from.GetCol()][to.GetRow()][to.GetCol()] * 100;
    }

    pos_and_score.push_back(std::make_tuple(i, score));
  }

  struct {
    bool operator()(std::tuple<size_t, int> a, std::tuple<size_t, int> b) {
      return std::get<1>(a) < std::get<1>(b);
    }
  } customLess;

  std::sort(pos_and_score.begin(), pos_and_score.end(), customLess);

  std::vector<Move> reordered_moves;
  reordered_moves.reserve(moves.size());
  for (size_t i = 0; i < moves.size(); i++) {
    const auto& p = pos_and_score[i];
    size_t pos = std::get<0>(p);
    reordered_moves.push_back(std::move(moves[pos]));
  }

  return reordered_moves;
}

int AlphaBetaPlayer::StaticExchangeEvaluation(
    const Board& board,
    const Move& move) const {

  const auto* captured = move.GetStandardCapture();

  std::vector<PlacedPiece> attackers_this_side = board.GetAttackers(
      board.GetTurn().GetTeam(), move.To());
  std::vector<PlacedPiece> attackers_that_side = board.GetAttackers(
      OtherTeam(board.GetTurn().GetTeam()), move.To());

  std::vector<int> piece_values_this_side;
  piece_values_this_side.reserve(attackers_this_side.size() - 1);
  std::vector<int> piece_values_that_side;
  piece_values_that_side.reserve(attackers_that_side.size());

  for (const auto& placed_piece : attackers_this_side) {
    if (placed_piece.GetLocation() != move.From()) {
      int piece_eval = piece_evaluations_[placed_piece.GetPiece()->GetPieceType()];
      piece_values_this_side.push_back(piece_eval);
    }
  }

  for (const auto& placed_piece : attackers_that_side) {
    int piece_eval = piece_evaluations_[placed_piece.GetPiece()->GetPieceType()];
    piece_values_that_side.push_back(piece_eval);
  }
  const auto* attacking = board.GetPiece(move.From());
  int attacking_piece_eval = piece_evaluations_[attacking->GetPieceType()];

  int value = piece_evaluations_[captured->GetPieceType()]
    - StaticExchangeEvaluation(attacking_piece_eval, piece_values_that_side, 0,
        piece_values_this_side, 0);


  return value;
}

int AlphaBetaPlayer::StaticExchangeEvaluation(
    int square_piece_eval,
    const std::vector<int>& sorted_piece_values,
    size_t pos,
    const std::vector<int>& other_team_sorted_piece_values,
    size_t other_team_pos) const {
  if (pos >= sorted_piece_values.size()) {
    return 0;
  }
  return square_piece_eval - StaticExchangeEvaluation(
      sorted_piece_values[pos],
      other_team_sorted_piece_values,
      other_team_pos,
      sorted_piece_values,
      pos + 1);
}

int PVInfo::GetDepth() const {
  if (best_move_.has_value()) {
    if (child_ == nullptr) {
      return 1;
    }
    return 1 + child_->GetDepth();
  }
  return 0;
}

}  // namespace chess

