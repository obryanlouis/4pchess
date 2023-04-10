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

// TODO: move these constants to a separate header file
constexpr int kMaxQuiescenceDepth = 8;
constexpr int kMaxQuiescenceMoves = 3;
constexpr int kMateValue = 1000000'00;  // mate value (centipawns)

AlphaBetaPlayer::AlphaBetaPlayer(std::optional<PlayerOptions> options) {
  if (options.has_value()) {
    options_ = options.value();
  }
  piece_evaluations_[PAWN] = 100;
  piece_evaluations_[KNIGHT] = 300;
  piece_evaluations_[BISHOP] = 400;
  piece_evaluations_[ROOK] = 500;
  piece_evaluations_[QUEEN] = 1000;
  piece_evaluations_[KING] = 10000;

  piece_move_order_scores_[PAWN] = 10;
  piece_move_order_scores_[KNIGHT] = 20;
  piece_move_order_scores_[BISHOP] = 30;
  piece_move_order_scores_[ROOK] = 40;
  piece_move_order_scores_[QUEEN] = 50;
  piece_move_order_scores_[KING] = 0;

  if (options_.enable_transposition_table) {
    transposition_table_ = std::make_unique<TranspositionTable>(
        options_.transposition_table_size);
  }

  for (int i = 1; i < kMaxPly; i++) {
    reductions_[i] = int(10.0 * std::log(i));
  }

  if (options_.enable_history_heuristic) {
    ResetHistoryHeuristic();
  }

  if (options_.enable_history_leaf_pruning) {
    ResetHistoryCounters();
  }
}

int AlphaBetaPlayer::Reduction(int depth, int move_number) const {
  return (reductions_[depth] * reductions_[move_number] + 1500) / 1000;
}


std::optional<int> AlphaBetaPlayer::QuiescenceSearch(
    Board& board,
    int depth,
    int alpha,
    int beta,
    bool maximizing_player,
    const std::optional<
        std::chrono::time_point<std::chrono::system_clock>>& deadline) {
  if (canceled_.load() || (deadline.has_value()
        && std::chrono::system_clock::now() >= deadline.value())) {
    // hit deadline
    return std::nullopt;
  }

  num_nodes_++;

  int eval = Evaluate(board, maximizing_player);
  if (depth <= 0) {
    return eval;
  }

  std::optional<int> value_or;
  Player player = board.GetTurn();
  bool checked = board.IsKingInCheck(player);

  if (!checked) {
    if (eval >= beta) {
      return beta;
    }
    if (alpha < eval) {
      alpha = eval;
    }
  }

  // TODO: use a pv move here, maybe
  std::vector<Move> pseudo_legal_moves = MoveOrder(
      board, maximizing_player, std::nullopt);

  int turn_i = static_cast<int>(player.GetColor());
  int curr_mob_score = player_mobility_scores_[turn_i];
  int num_moves_examined = 0;

  for (int i = pseudo_legal_moves.size() - 1;
      i >= 0 && num_moves_examined < kMaxQuiescenceMoves; i--) {
    const auto& move = pseudo_legal_moves[i];
    bool active = checked
               || board.DeliversCheck(move)
               || (move.GetStandardCapture() != nullptr
                   && StaticExchangeEvaluationCapture(board, move) > 0);

    board.MakeMove(move);

    if (board.CheckWasLastMoveKingCapture() != IN_PROGRESS) {
      num_moves_examined++;
      board.UndoMove();
      if (options_.enable_mobility_evaluation) {
        player_mobility_scores_[turn_i] = curr_mob_score; // reset
      }

      return kMateValue;
    }

    if (board.IsKingInCheck(player)) { // Illegal move
      board.UndoMove();

      continue;
    }

    if (!active) {
      board.UndoMove();
      continue;
    }

    if (options_.enable_mobility_evaluation) {
      int player_mobility_score = board.MobilityEvaluation(player);
      player_mobility_scores_[turn_i] = player_mobility_score;
    }

    value_or = QuiescenceSearch(
        board, depth - 1, -beta, -alpha, !maximizing_player,
        deadline);

    board.UndoMove();

    num_moves_examined++;

    if (!value_or.has_value()) {
      return std::nullopt; // timeout
    }

    int score = -value_or.value();
    if (score >= beta) {
      if (options_.enable_mobility_evaluation) {
        player_mobility_scores_[turn_i] = curr_mob_score; // reset
      }

      return beta;  // cutoff
    }
    if (score > alpha) {
      alpha = score;
    }
  }

  if (options_.enable_mobility_evaluation) {
    player_mobility_scores_[turn_i] = curr_mob_score; // reset
  }

  return alpha;
}


// Alpha-beta search with nega-max framework.
// https://www.chessprogramming.org/Alpha-Beta
// Returns (nega-max value, best move) pair.
// The best move is nullopt if the game is over.
// If the function returns std::nullopt, then it hit the deadline
// before finishing search and the results should not be used.
std::optional<std::tuple<int, std::optional<Move>>> AlphaBetaPlayer::Search(
    Stack* ss,
    NodeType node_type,
    Board& board,
    int ply,
    int depth,
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
  num_nodes_++;
  //int alpha_orig = alpha;
  bool is_root_node = ply == 1;

  bool is_pv_node = node_type != NonPV;

  std::optional<Move> tt_move;
  if (options_.enable_transposition_table) {
    int64_t key = board.HashKey();
    auto* entry = transposition_table_->Get(key);
    if (entry != nullptr && entry->key == key) { // valid entry
      if (entry->depth >= depth) {
        num_cache_hits_++;
        // at non-PV nodes check for an early TT cutoff
        if (!is_root_node
            && !is_pv_node
            && (entry->bound == EXACT
              || (entry->bound == LOWER_BOUND && entry->score >= beta)
              || (entry->bound == UPPER_BOUND && entry->score <= alpha))
           ) {

          if (entry->move.has_value()
              && !entry->move->IsCapture()) {
            UpdateQuietStats(ss, entry->move.value());
          }

          return std::make_tuple(
              std::min(beta, std::max(alpha, entry->score)), entry->move);
        }
      }
      tt_move = entry->move;
    }
  }

  int eval = Evaluate(board, maximizing_player);

  if (depth <= 0 || ply >= kMaxPly) {

    if (options_.enable_quiescence) {

      auto value_or = QuiescenceSearch(
          board, kMaxQuiescenceDepth, alpha, beta, maximizing_player, deadline);
      if (!value_or.has_value()) {
        return std::nullopt; // timeout
      }
      eval = value_or.value();
    }

    if (options_.enable_transposition_table) {
      transposition_table_->Save(board.HashKey(), 0, std::nullopt, eval, EXACT);
    }

    return std::make_tuple(eval, std::nullopt);
  }

  Player player = board.GetTurn();

//  // futility pruning
//  if (options_.enable_futility_pruning
//      && !is_pv_node // not a pv node
//      && ply > 1 // not root
//      && ply < 9 // important for mate finding
//      && eval >= beta + 150 * depth
//      ) {
//    num_futility_moves_pruned_++;
//    return std::make_pair(beta, std::nullopt);
//  }

  // null move pruning
  if (options_.enable_null_move_pruning
      && !is_root_node // not root
      && !is_pv_node // not a pv node
      && null_moves == 0 // last move wasn't null
      && !board.IsKingInCheck(player) // not in check
      && eval >= beta
      ) {
    num_null_moves_tried_++;
    board.MakeNullMove();

    // try the null move with possibly reduced depth
    PVInfo null_pvinfo;
    int r = std::min(depth / 3 + 1, depth - 1);
    auto value_and_move_or = Search(
        ss+1, NonPV, board, ply + 1, depth - r - 1,
        -beta, -beta + 1, !maximizing_player, expanded, deadline, null_pvinfo,
        null_moves + 1);

    board.UndoNullMove();

    // if it failed high, skip this move
    if (value_and_move_or.has_value()
        && -std::get<0>(value_and_move_or.value()) >= beta) {
      num_null_moves_pruned_++;

      if (options_.enable_transposition_table) {
        transposition_table_->Save(board.HashKey(), depth, std::nullopt, beta, LOWER_BOUND);
      }

      return std::make_tuple(beta, std::nullopt);
    }
  }

  std::vector<Move> pseudo_legal_moves;
  std::optional<Move> pv_move = pvinfo.GetBestMove();
  pseudo_legal_moves = MoveOrder(
      board, maximizing_player, pv_move.has_value() ? pv_move : tt_move,
      ss->killers);

  //int value = -kMateValue;
  std::optional<Move> best_move;
  int player_color = static_cast<int>(player.GetColor());
  int curr_mob_score = player_mobility_scores_[player_color];

//  if (options_.enable_extensions) {
//    // mate threat extension
//    if (null_moves == 0
//        && depth <= 1
//        && expanded <= 0) {
//      // check if the current player would be mated after a null move
//      // (blocks checkmates)
//      int mate_beta = kMateValue/2;
//      board.MakeNullMove();
//      PVInfo null_pvinfo;
//      auto zw_value_or = Search(
//          board, ply + 1, 4, mate_beta - 1, mate_beta,
//          !maximizing_player, expanded+4, deadline, null_pvinfo,
//          null_moves + 1);
//      board.UndoNullMove();
//      if (zw_value_or.has_value()
//          && mate_beta < std::get<0>(zw_value_or.value())) { // extend
//        extension = std::max(extension, 4);
//      }
//    }
//  }

  //bool b_search_pv = true;

  bool has_legal_moves = false;
  int move_count = 0;
  for (int i = pseudo_legal_moves.size() - 1; i >= 0; i--) {
    std::optional<std::tuple<int, std::optional<Move>>> value_and_move_or;
    const auto& move = pseudo_legal_moves[i];
//    const auto* piece = board.GetPiece(move.From());

    bool delivers_check = false;
    bool lmr_cond1 =
      options_.enable_late_move_reduction
      && depth > 1
      && !is_pv_node
      && move_count > 2
      && !move.IsCapture()
      ;
    if (lmr_cond1) {
      // this has to be called before the move is made
      delivers_check = board.DeliversCheck(move);
    }

//    bool positive_see = false;
//    if (options_.enable_quiescence
//        && move.GetStandardCapture() != nullptr
//        && depth < 4) {
//      int see = StaticExchangeEvaluationCapture(board, move);
//      positive_see = see > 0;
//    }

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

    move_count++;
    has_legal_moves = true;

    if (options_.enable_mobility_evaluation) {
      int player_mobility_score = board.MobilityEvaluation(player);
      player_mobility_scores_[player_color] = player_mobility_score;
    }

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

    int e = 0;  // extension
    if (options_.enable_extensions) {

//      if (options_.enable_quiescence
//          && move.GetStandardCapture() != nullptr
//          && depth < 4
//          && !positive_see) {
//        e++;
//      }

//      // works, but is too expensive
//      // mate threat extension
//      if (null_moves == 0
//          && move.IsCapture()
//          && depth <= 1
//          && expanded <= 0
//          && e <= 0) {
//        // check if the other team would be mated after a null move
//        // (finds checkmates)
//        int mate_beta = kMateValue/2;
//        board.MakeNullMove();  // next player makes null move
//        PVInfo null_pvinfo;
//        auto zw_value_or = Search(
//            board, ply + 1, 2, mate_beta - 1, mate_beta,
//            maximizing_player, expanded+2, deadline, null_pvinfo,
//            null_moves + 1);
//        board.UndoNullMove();
//        if (zw_value_or.has_value()
//            && mate_beta < std::get<0>(zw_value_or.value())) { // extend
//          e = std::max(e, 3);
//        }
//      }

    }

    bool lmr = lmr_cond1
        && !delivers_check
        && !board.IsKingInCheck(player.GetTeam());

    if (lmr) {
      num_lmr_searches_++;

      int r = Reduction(depth, move_count + 1);
      r = std::clamp(r, 0, depth - 2);
      value_and_move_or = Search(
          ss+1, NonPV, board, ply + 1, depth - 1 - r + e,
          -alpha-1, -alpha, !maximizing_player, expanded + e,
          deadline, *child_pvinfo, null_moves);
      if (value_and_move_or.has_value()) {
        int score = -std::get<0>(value_and_move_or.value());
        if (score > alpha) {  // re-search
          num_lmr_researches_++;
          value_and_move_or = Search(
              ss+1, NonPV, board, ply + 1, depth - 1 + e,
              -beta, -alpha, !maximizing_player, expanded + e,
              deadline, *child_pvinfo, null_moves);
        }
      }

    } else if (!is_pv_node || move_count > 1) {
      value_and_move_or = Search(
          ss+1, NonPV, board, ply + 1, depth - 1 + e,
          -alpha-1, -alpha, !maximizing_player, expanded + e,
          deadline, *child_pvinfo, null_moves);
    }

    // For PV nodes only, do a full PV search on the first move or after a fail
    // high (in the latter case search only if value < beta), otherwise let the
    // parent node fail low with value <= alpha and try another move.
    if (is_pv_node && (move_count == 1 || (value_and_move_or.has_value() && (
            -std::get<0>(value_and_move_or.value()) > alpha
            && (is_root_node
              || -std::get<0>(value_and_move_or.value()) < beta))))) {
        value_and_move_or = Search(
            ss+1, PV, board, ply + 1, depth - 1 + e,
            -beta, -alpha, !maximizing_player, expanded + e,
            deadline, *child_pvinfo, null_moves);
    }

    board.UndoMove();

    if (options_.enable_mobility_evaluation) { // reset
      player_mobility_scores_[player_color] = curr_mob_score;
    }

    if (!value_and_move_or.has_value()) {
      return std::nullopt; // timeout
    }
    int score = -std::get<0>(value_and_move_or.value());

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
            [to.GetRow()][to.GetCol()] += (1 << depth);
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
      //b_search_pv = false;
    }
    if (!best_move.has_value()) {
      best_move = move;
    }
  }

  int score = alpha;
  if (!has_legal_moves) {
    if (!board.IsKingInCheck(board.GetTurn())) {
      // stalemate
      score = std::min(beta, std::max(alpha, 0));
    } else {
      // checkmate
      score = std::min(beta, std::max(alpha, -kMateValue));
    }
  }

  if (options_.enable_transposition_table) {
    ScoreBound bound = beta <= alpha ? LOWER_BOUND : is_pv_node &&
      best_move.has_value() ? EXACT : UPPER_BOUND;
    transposition_table_->Save(board.HashKey(), depth, best_move, score, bound);
  }

  if (best_move.has_value()
      && !best_move->IsCapture()) {
    UpdateQuietStats(ss, *best_move);
  }

  return std::make_tuple(score, best_move);
}

void AlphaBetaPlayer::UpdateQuietStats(Stack* ss, const Move& move) {
  if (options_.enable_killers) {
    if (ss->killers[0] != move) {
      ss->killers[1] = ss->killers[0];
      ss->killers[0] = move;
    }
  }
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
  Stack stack[kMaxPly + 10];

  std::optional<std::tuple<int, std::optional<Move>>> move_and_value;
  std::optional<Move> best_move;
  while (lower_bound < upper_bound && (!deadline.has_value()
        || std::chrono::system_clock::now() < deadline.value())) {
    int b = std::max(g, lower_bound + 1);
    move_and_value = Search(
        stack, Root, board, 1, depth, b - 1, b, maximizing_player, 0,
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
  Stack stack[kMaxPly + 10];

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
          stack, Root, board, 1, next_depth, alpha, beta, maximizing_player, 0,
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
    const std::optional<Move>& pvmove,
    Move* killers) {

  if (!options_.enable_move_order) {
    return board.GetPseudoLegalMoves();
  }

  auto moves = board.GetPseudoLegalMoves();
  // reorder the moves
  std::vector<std::tuple<size_t, int>> pos_and_score;
  pos_and_score.reserve(moves.size());

  // move order:
  // 1. PV or TT move
  // 2. Good captures
  // 3. Killers
  // 4. Quiet moves
  // 5. Bad captures
  constexpr int kSep = 1000000;
  constexpr int kPVMoveScore = 10 * kSep;
  constexpr int kGoodCaptureScore = 9 * kSep;
  constexpr int kKillerScore = 8 * kSep;
  constexpr int kQuietMoveScore = 7 * kSep;
  constexpr int kBadCaptureScore = 6 * kSep;

  for (size_t i = 0; i < moves.size(); i++) {
    const auto& move = moves[i];
    int score = 0;

    const auto* capture = move.GetCapturePiece();
    const auto* piece = board.GetPiece(move.From());

    if (pvmove.has_value() && move == pvmove.value()) {
      score = kPVMoveScore;
    } else if (killers != nullptr
               && (killers[0] == move || killers[1] == move)) {
      score = kKillerScore + (move == killers[0] ? 1 : 0);
    } else if (move.IsCapture()) {
      // We'd ideally use SEE here, if it weren't so expensive to compute.
      int captured_val = piece_evaluations_[capture->GetPieceType()];
      int attacker_val = piece_evaluations_[piece->GetPieceType()];
      if (attacker_val >= captured_val) {
        score = kGoodCaptureScore;
      } else {
        score = kBadCaptureScore;
      }
      score += captured_val - attacker_val;
    } else {
      score = kQuietMoveScore;
      if (options_.enable_history_heuristic) {
        const auto& from = move.From();
        const auto& to = move.To();
        score += history_heuristic_[from.GetRow()][from.GetCol()][to.GetRow()][to.GetCol()];
      }
    }

    if (options_.enable_move_order_checks) {
      if (board.DeliversCheck(move)) {
        score += 10'00;
      }
    }

    score += piece_move_order_scores_[piece->GetPieceType()];

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

int AlphaBetaPlayer::StaticExchangeEvaluationCapture(
    Board& board,
    const Move& move) const {

  const auto* captured = move.GetStandardCapture();
  assert(captured != nullptr);

  int value = piece_evaluations_[captured->GetPieceType()];

  board.MakeMove(move);
  value -= StaticExchangeEvaluation(board, move.To());
  board.UndoMove();
  return value;
}

int AlphaBetaPlayer::StaticExchangeEvaluation(
      const Board& board, const BoardLocation& loc) const {
  std::vector<PlacedPiece> attackers_this_side = board.GetAttackers(
      board.GetTurn().GetTeam(), loc);
  std::vector<PlacedPiece> attackers_that_side = board.GetAttackers(
      OtherTeam(board.GetTurn().GetTeam()), loc);

  std::vector<int> piece_values_this_side;
  piece_values_this_side.reserve(attackers_this_side.size());
  std::vector<int> piece_values_that_side;
  piece_values_that_side.reserve(attackers_that_side.size());

  for (const auto& placed_piece : attackers_this_side) {
    int piece_eval = piece_evaluations_[placed_piece.GetPiece()->GetPieceType()];
    piece_values_this_side.push_back(piece_eval);
  }

  for (const auto& placed_piece : attackers_that_side) {
    int piece_eval = piece_evaluations_[placed_piece.GetPiece()->GetPieceType()];
    piece_values_that_side.push_back(piece_eval);
  }

  std::sort(piece_values_this_side.begin(), piece_values_this_side.end());
  std::sort(piece_values_that_side.begin(), piece_values_that_side.end());

  const auto* attacking = board.GetPiece(loc);
  assert(attacking != nullptr);
  int attacked_piece_eval = piece_evaluations_[attacking->GetPieceType()];

  return StaticExchangeEvaluation(
      attacked_piece_eval,
      piece_values_this_side,
      0,
      piece_values_that_side,
      0);
}

int AlphaBetaPlayer::StaticExchangeEvaluation(
    int square_piece_eval,
    const std::vector<int>& sorted_piece_values,
    size_t index,
    const std::vector<int>& other_team_sorted_piece_values,
    size_t other_index) const {
  if (index >= sorted_piece_values.size()) {
    return 0;
  }
  int value_capture = square_piece_eval - StaticExchangeEvaluation(
      sorted_piece_values[index],
      other_team_sorted_piece_values,
      other_index,
      sorted_piece_values,
      index + 1);
  return std::max(0, value_capture);
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

