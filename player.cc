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
constexpr int kMaxQuiescenceDepth = 4;
constexpr int kMaxQuiescenceMoves = 2;
constexpr int kMateValue = 1000000'00;  // mate value (centipawns)

AlphaBetaPlayer::AlphaBetaPlayer(std::optional<PlayerOptions> options) {
  if (options.has_value()) {
    options_ = options.value();
  }
  piece_evaluations_[PAWN] = 50;
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

  for (int row = 0; row < 14; row++) {
    for (int col = 0; col < 14; col++) {
      if (row <= 2 || row >= 11 || col <= 2 || col >= 11) {
        location_evaluations_[row][col] = 5;
      } else if (row <= 4 || row >= 9 || col <= 4 || col >= 9) {
        location_evaluations_[row][col] = 10;
      } else {
        location_evaluations_[row][col] = 15;
      }
    }
  }

  king_attack_weight_[0] = 0;
  king_attack_weight_[1] = 0;
  king_attack_weight_[2] = 50;
  king_attack_weight_[3] = 75;
  king_attack_weight_[4] = 88;
  king_attack_weight_[5] = 94;
  king_attack_weight_[6] = 97;
  king_attack_weight_[7] = 99;
  for (int i = 8; i < 30; i++) {
    king_attack_weight_[i] = 100;
  }

  late_move_pruning_[0] = 0;
  late_move_pruning_[1] = 2;
  late_move_pruning_[2] = 2;
  late_move_pruning_[3] = 3;
  late_move_pruning_[4] = 3;
  late_move_pruning_[5] = 5;
  late_move_pruning_[6] = 5;
  late_move_pruning_[7] = 6;
  late_move_pruning_[8] = 6;
  late_move_pruning_[9] = 7;
  late_move_pruning_[10] = 7;
  late_move_pruning_[11] = 8;
  late_move_pruning_[12] = 8;
  late_move_pruning_[13] = 9;
  late_move_pruning_[14] = 9;
  late_move_pruning_[15] = 10;
  late_move_pruning_[16] = 10;
  late_move_pruning_[17] = 10;
  late_move_pruning_[18] = 10;
  late_move_pruning_[19] = 10;

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
        std::chrono::time_point<std::chrono::system_clock>>& deadline,
    int msla) {
  if (canceled_.load() || (deadline.has_value()
        && std::chrono::system_clock::now() >= deadline.value())) {
    // hit deadline
    return std::nullopt;
  }

  num_nodes_++;
  num_quiescence_nodes_++;

  int eval = Evaluate(board, maximizing_player);
  if (depth <= 0 || msla >= 4) {
    return eval;
  }

  std::optional<int> value_or;
  Player player = board.GetTurn();

  bool checked = board.IsKingInCheck(player.GetTeam());

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
      board, maximizing_player, std::nullopt, nullptr, true);

  int turn_i = static_cast<int>(player.GetColor());
  int curr_mob_score = player_mobility_scores_[turn_i];
  int num_moves_examined = 0;

  for (int i = pseudo_legal_moves.size() - 1;
      i >= 0 && num_moves_examined < kMaxQuiescenceMoves; i--) {
    auto& move = pseudo_legal_moves[i];
    bool active = checked
               || move.DeliversCheck(board)
               || (move.GetStandardCapture() != nullptr
                   // approx SEE >= 0
//                   && piece_evaluations_[board.GetPiece(move.From())->GetPieceType()]
//                      < piece_evaluations_[board.GetPiece(move.To())->GetPieceType()])
                   && StaticExchangeEvaluationCapture(board, move) > 0)
               ;

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
        deadline, 0);

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

//  if (num_moves_examined == 0) {
//    // if there were no captures to try, force a null move
//    board.MakeNullMove();
//
//    value_or = QuiescenceSearch(board, depth - 1, -beta, -alpha,
//        !maximizing_player, deadline, msla + 1);
//
//    board.UndoNullMove();
//
//    if (!value_or.has_value()) {
//      return std::nullopt; // timeout
//    }
//    int score = -value_or.value();
//    alpha = std::max(alpha, score);
//  }

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
    int null_moves,
    bool isCutNode) {
  if (canceled_.load() || (deadline.has_value()
        && std::chrono::system_clock::now() >= deadline.value())) {
    return std::nullopt;
  }
  num_nodes_++;
  //int alpha_orig = alpha;
  bool is_root_node = ply == 1;

  bool is_pv_node = node_type != NonPV;
  bool is_tt_pv = false;

  std::optional<Move> tt_move;
  const HashTableEntry* tte = nullptr;
  if (options_.enable_transposition_table) {
    int64_t key = board.HashKey();
    tte = transposition_table_->Get(key);
    if (tte != nullptr && tte->key == key) { // valid entry
      if (tte->depth >= depth) {
        num_cache_hits_++;
        // at non-PV nodes check for an early TT cutoff
        if (!is_root_node
            && !is_pv_node
            && (tte->bound == EXACT
              || (tte->bound == LOWER_BOUND && tte->score >= beta)
              || (tte->bound == UPPER_BOUND && tte->score <= alpha))
           ) {

          if (tte->move.has_value()
              && !tte->move->IsCapture()) {
            UpdateQuietStats(ss, tte->move.value());
          }

          return std::make_tuple(
              std::min(beta, std::max(alpha, tte->score)), tte->move);
        }
      }
      tt_move = tte->move;
      is_tt_pv = tte->is_pv;
    }
  }

  int eval = Evaluate(board, maximizing_player);

  Player player = board.GetTurn();
  //Team team = player.GetTeam();

  if (depth <= 0 || ply >= kMaxPly) {

    if (options_.enable_quiescence) {

      num_nodes_--; // added in the next call
      auto value_or = QuiescenceSearch(
          board, kMaxQuiescenceDepth, alpha, beta, maximizing_player, deadline);
      if (!value_or.has_value()) {
        return std::nullopt; // timeout
      }
      eval = value_or.value();
    }

    if (options_.enable_transposition_table) {
      transposition_table_->Save(board.HashKey(), 0, std::nullopt, eval, EXACT, is_pv_node);
    }

    return std::make_tuple(eval, std::nullopt);
  }

  if (!ss->excluded_move.has_value()) {
    ss->tt_pv = is_pv_node || (tte != nullptr && tte->is_pv);
  }
  (ss+1)->excluded_move = std::nullopt;
  (ss+2)->killers[0] = (ss+2)->killers[1] = Move();


  // futility pruning
  if (options_.enable_futility_pruning
      && !is_pv_node // not a pv node
      && !is_tt_pv // not TT pv
      && !is_root_node // not root
      && depth < 9 // important for mate finding
      && eval >= beta + 150 * depth
      && eval < 25000
      ) {
    num_futility_moves_pruned_++;
    return std::make_pair(beta, std::nullopt);
  }

  bool in_check = board.IsKingInCheck(player);

  // null move pruning
  if (options_.enable_null_move_pruning
      && !is_root_node // not root
      && !ss->excluded_move.has_value()
      && !is_pv_node // not a pv node
      && null_moves == 0 // last move wasn't null
      && !in_check // not in check
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
        transposition_table_->Save(board.HashKey(), depth, std::nullopt, beta, LOWER_BOUND, is_pv_node);
      }

      return std::make_tuple(beta, std::nullopt);
    }
  }

  // fail-high reductions
  if (options_.enable_fail_high_reductions
      && !is_pv_node // not a pv node
      && !is_tt_pv // not TT pv
      && !is_root_node // not root
      && eval >= beta
      && !in_check
      && isCutNode
      && depth > 10 // insure we are at a high enough depth
      && beta > -2000
      ) {
    num_fail_high_reductions_++;
    depth--;
  }

  std::vector<Move> pseudo_legal_moves;
  std::optional<Move> pv_move = pvinfo.GetBestMove();
  if (!options_.enable_move_order) {
    pseudo_legal_moves = board.GetPseudoLegalMoves();
  } else {
    pseudo_legal_moves = MoveOrder(
        board, maximizing_player, pv_move.has_value() ? pv_move : tt_move,
        ss->killers);
  }

  bool partner_checked = false;
  if (options_.enable_late_move_reduction) {
    partner_checked = board.IsKingInCheck(GetPartner(player));
  }

  std::optional<Move> best_move;
  int player_color = static_cast<int>(player.GetColor());
  int curr_mob_score = player_mobility_scores_[player_color];

  bool has_legal_moves = false;
  int move_count = 0;
  int quiets = 0;

  for (int i = pseudo_legal_moves.size() - 1; i >= 0; i--) {
    std::optional<std::tuple<int, std::optional<Move>>> value_and_move_or;
    auto& move = pseudo_legal_moves[i];
    if (move == ss->excluded_move) {
      continue;
    }

    bool delivers_check = false;
    bool lmr_cond1 =
      options_.enable_late_move_reduction
      && depth > 1
      && !is_pv_node
      && move_count > 2
      && !move.IsCapture()
      ;
    if (lmr_cond1 || options_.enable_late_move_pruning) {
      // this has to be called before the move is made
      delivers_check = move.DeliversCheck(board);
    }

    bool quiet = !in_check && !move.IsCapture() && !delivers_check;

    if (options_.enable_late_move_pruning
        && quiet
        && !is_tt_pv
        && !is_pv_node
        && depth <= 14
        && quiets >= late_move_pruning_[depth]) {
      num_lm_pruned_++;
      continue;
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

    has_legal_moves = true;

//    // futility pruning
//    if (options_.enable_futility_pruning
//        && !is_pv_node // not a pv node
//        && !is_tt_pv // not TT pv
//        && !is_root_node // not root
//        && depth == 1
//        //&& eval >= beta + 150 * depth
//        && eval + 150*depth < alpha
//        && !move.IsCapture()
//        ) {
//      board.UndoMove();
//      num_futility_moves_pruned_++;
//      continue;
//    }

    move_count++;
    if (quiet) {
      quiets++;
    }

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

    if (ply < root_depth_ * 2) {
      // Singular extension search
      if (options_.enable_singular_extensions
          && !is_root_node
          && !ss->excluded_move.has_value()  // avoid recursive singular search
          && tte != nullptr
          && depth >= 3 + 2 * (is_pv_node && is_tt_pv)
          && move == tt_move
          && std::abs(tte->score) < kMateValue
          && tte->bound == LOWER_BOUND
          && tte->depth >= depth - 3
          ) {

        int singular_beta = tte->score;
        int singular_depth = (depth - 1) / 2;

        num_singular_extension_searches_++;
        board.UndoMove();
        ss->excluded_move = move;
        PVInfo singular_pvinfo;
        auto res_or = Search(ss, NonPV, board, ply, singular_depth,
            singular_beta - 1, singular_beta, maximizing_player, expanded,
            deadline, singular_pvinfo, null_moves);
        if (!res_or.has_value()) {
          board.UndoMove();
          return std::nullopt;
        }
        ss->excluded_move = std::nullopt;

        int value = std::get<0>(res_or.value());
        if (value < singular_beta) {
          num_singular_extensions_++;
          e = 1;
        } else if (singular_beta >= beta) {
          // multi-cut pruning
          return std::make_pair(beta, std::nullopt);
        } else if (tte->score >= beta) {
          e = -2 - !is_pv_node;
        } else if (tte->score <= value) {
          e = -1;
        } else if (tte->score <= alpha) {
          e = -1;
        }

        // Remake the move
        board.MakeMove(move);
      }

    }

    // check extensions at early moves.
    if (options_.enable_check_extensions
        && in_check
        && move_count < 6
        && expanded < 3) {
      num_check_extensions_++;
      e = 1;
    }

    bool lmr = lmr_cond1
        && !delivers_check
        && !in_check
        && !partner_checked;

    if (lmr) {
      num_lmr_searches_++;

      int r = Reduction(depth, move_count + 1);
      r = std::clamp(r, 0, depth - 2);
      value_and_move_or = Search(
          ss+1, NonPV, board, ply + 1, depth - 1 - r + e,
          -alpha-1, -alpha, !maximizing_player, expanded + e,
          deadline, *child_pvinfo, null_moves, !isCutNode);
      if (value_and_move_or.has_value()) {
        int score = -std::get<0>(value_and_move_or.value());
        if (score > alpha) {  // re-search
          num_lmr_researches_++;
          value_and_move_or = Search(
              ss+1, NonPV, board, ply + 1, depth - 1 + e,
              -beta, -alpha, !maximizing_player, expanded + e,
              deadline, *child_pvinfo, null_moves, !isCutNode);
        }
      }

    } else if (!is_pv_node || move_count > 1) {
      value_and_move_or = Search(
          ss+1, NonPV, board, ply + 1, depth - 1 + e,
          -alpha-1, -alpha, !maximizing_player, expanded + e,
          deadline, *child_pvinfo, null_moves, !isCutNode);
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
            deadline, *child_pvinfo, null_moves, true);
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
    }
    if (!best_move.has_value()) {
      best_move = move;
    }
  }

  int score = alpha;
  if (!has_legal_moves) {
    if (!in_check) {
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
    transposition_table_->Save(board.HashKey(), depth, best_move, score, bound, is_pv_node);
  }

  if (best_move.has_value()
      && !best_move->IsCapture()) {
    UpdateQuietStats(ss, *best_move);
  }

  // If no good move is found and the previous position was tt_pv, then the
  // previous opponent move is probably good and the new position is added to
  // the search tree.
  if (score <= alpha) {
    ss->tt_pv = ss->tt_pv || ((ss-1)->tt_pv && depth > 3);
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

namespace {

std::tuple<int, int> GetPieceStatistics(
    const std::vector<PlacedPiece>& pieces,
    PlayerColor color) {
  int num_major = 0;
  float sum_center_dist = 0;
  for (const auto& placed_piece : pieces) {
    PieceType pt = placed_piece.GetPiece()->GetPieceType();
    if (pt != PAWN && pt != KING) {
      num_major++;
      const auto& loc = placed_piece.GetLocation();
      int row = loc.GetRow();
      int col = loc.GetCol();
      float center_dist = std::sqrt((row - 6.5) * (row - 6.5) + (col - 6.5) * (col - 6.5));
      sum_center_dist += center_dist;
    }
  }
  float avg_center_dist = sum_center_dist / std::max(num_major, 1);
  return std::make_tuple(num_major, avg_center_dist);
}

}  // namespace

int AlphaBetaPlayer::Evaluate(Board& board, bool maximizing_player) {
  int eval; // w.r.t. RY team
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
    // Piece evaluation
    eval = board.PieceEvaluation();

    if (options_.enable_piece_imbalance
        || options_.enable_piece_development) {
      const auto& piece_list = board.GetPieceList();
      auto red_stats = GetPieceStatistics(piece_list[RED], RED);
      auto blue_stats = GetPieceStatistics(piece_list[BLUE], BLUE);
      auto yellow_stats = GetPieceStatistics(piece_list[YELLOW], YELLOW);
      auto green_stats = GetPieceStatistics(piece_list[GREEN], GREEN);

      // Piece imbalance
      if (options_.enable_piece_imbalance) {
        int red_piece_eval = board.PieceEvaluation(RED);
        int yellow_piece_eval = board.PieceEvaluation(YELLOW);
        int blue_piece_eval = board.PieceEvaluation(BLUE);
        int green_piece_eval = board.PieceEvaluation(GREEN);

        int ry_diff = std::abs(red_piece_eval - yellow_piece_eval);
        eval -= ry_diff/3;
        int bg_diff = std::abs(blue_piece_eval - green_piece_eval);
        eval += bg_diff/3;
      }

      // Piece development
      if (options_.enable_piece_development) {
        eval -= 50 * (std::get<1>(red_stats) + std::get<1>(yellow_stats)
          - std::get<1>(blue_stats) - std::get<1>(green_stats));
      }

    }

    // Mobility evaluation
    if (options_.enable_mobility_evaluation) {
      for (int i = 0; i < 4; i++) {
        eval += player_mobility_scores_[i];
      }
    }

    // King safety evaluation
    if (options_.enable_king_safety) {
      for (int color = 0; color < 4; ++color) {
        int king_safety = 0;
        Player player(static_cast<PlayerColor>(color));
        Team team = player.GetTeam();
        Team other = OtherTeam(team);
        const auto king_location_or = board.GetKingLocation(player);
        if (king_location_or.has_value()) {
          const auto& king_location = king_location_or.value();
          for (int delta_row = -1; delta_row <= 1; ++delta_row) {
            for (int delta_col = -1; delta_col <= 1; ++delta_col) {
              int row = king_location.GetRow() + delta_row;
              int col = king_location.GetCol() + delta_col;
              if ((row == 1 || row == 12 || col == 1 || col == 12)
                  && board.IsLegalLocation(row, col)) {
                const auto* piece = board.GetPiece(row, col);
                BoardLocation piece_location(row, col);
                if (piece != nullptr
                    && piece->GetColor() == color
                    && piece->GetPieceType() == PAWN) {
                  king_safety += 30;
                }
                auto attackers = board.GetAttackers(other, piece_location);
                if (attackers.size() > 1) {
                  int value_of_attacks = 0;
                  for (const auto& placed_piece : attackers) {
                    switch(placed_piece.GetPiece()->GetPieceType()) {
                    case KNIGHT:
                      value_of_attacks += 20;
                      break;
                    case BISHOP:
                      value_of_attacks += 30;
                      break;
                    case ROOK:
                      value_of_attacks += 40;
                      break;
                    case QUEEN:
                      value_of_attacks += 80;
                      break;
                    default:
                      break;
                    }
                  }
                  king_safety -= value_of_attacks * king_attack_weight_[attackers.size()] / 100;
                }
              }
            }
          }
          const auto& castling_rights = board.GetCastlingRights(player);
          if (!castling_rights.Kingside() && !castling_rights.Queenside()) {
            king_safety -= 50;
          }
        }

        if (color == RED || color == YELLOW) {
          eval += king_safety;
        } else {
          eval -= king_safety;
        }
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
  Stack* ss = stack + 7;

  std::optional<std::tuple<int, std::optional<Move>>> move_and_value;
  std::optional<Move> best_move;
  while (lower_bound < upper_bound && (!deadline.has_value()
        || std::chrono::system_clock::now() < deadline.value())) {
    int b = std::max(g, lower_bound + 1);
    move_and_value = Search(
        ss, Root, board, 1, depth, b - 1, b, maximizing_player, 0,
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

  int next_depth = std::min(1 + pv_info_.GetDepth(), max_depth);
  std::optional<std::tuple<int, std::optional<Move>>> res;
  int alpha = -kMateValue;
  int beta = kMateValue;
  bool maximizing_player = board.TeamToPlay() == RED_YELLOW;
  int searched_depth = 0;
  Stack stack[kMaxPly + 10];
  Stack* ss = stack + 7;

  while (next_depth <= max_depth) {
    std::optional<std::tuple<int, std::optional<Move>>> move_and_value;
    root_depth_ = next_depth;

    if (options_.enable_mtdf) {
      int f = 0;
      if (res.has_value()) {
        f = std::get<0>(res.value());
      }
      move_and_value = MTDF(board, deadline, next_depth, f);
    } else {
      move_and_value = Search(
          ss, Root, board, 1, next_depth, alpha, beta, maximizing_player, 0,
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
    Move* killers,
    bool quiescence) {

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
  // 4. Bad captures
  // 5. Quiet moves
  int kSep = 1000000;
  int kPVMoveScore = 10 * kSep;
  int kGoodCaptureScore = 9 * kSep;
  int kKillerScore = 8 * kSep;
  int kBadCaptureScore = 7 * kSep;
  int kQuietMoveScore = 6 * kSep;

  for (size_t i = 0; i < moves.size(); i++) {
    auto& move = moves[i];
    int score = 0;

    const auto* capture = move.GetCapturePiece();
    const auto* piece = board.GetPiece(move.From());

    bool is_pv_move = pvmove.has_value() && move == pvmove.value();
    if (pvmove.has_value() && move == pvmove.value()) {
      score = kPVMoveScore;
    } else if (killers != nullptr
               && (killers[0] == move || killers[1] == move)) {
      score = kKillerScore + (move == killers[0] ? 1 : 0);
    } else if (move.IsCapture()) {

      if (options_.enable_see_move_ordering) {

        int see;
        if (board.GetPiece(move.From())->GetPieceType() == PAWN) {
          see = piece_evaluations_[capture->GetPieceType()];
        } else {
          see = StaticExchangeEvaluationCapture(board, move);
        }
        if (see >= 0) {
          score = kGoodCaptureScore;
        } else {
          score = kBadCaptureScore;
        }
        score += see;

      } else {

        // We'd ideally use SEE here, if it weren't so expensive to compute.
        int captured_val = piece_evaluations_[capture->GetPieceType()];
        int attacker_val = piece_evaluations_[piece->GetPieceType()];
        if (attacker_val <= captured_val) {
          score = kGoodCaptureScore;
        } else {
          score = kBadCaptureScore;
        }
        score += captured_val - attacker_val/100;

      }

    } else {
      score = kQuietMoveScore;
      if (options_.enable_history_heuristic) {
        const auto& from = move.From();
        const auto& to = move.To();
        score += history_heuristic_[from.GetRow()][from.GetCol()][to.GetRow()][to.GetCol()];
      }
    }

    if (options_.enable_move_order_checks
        && !is_pv_move
        && move.DeliversCheck(board)) {
      score += 10'00;
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

int AlphaBetaPlayer::ApproxSEECapture(
    Board& board, const Move& move) const {
  return piece_evaluations_[board.GetPiece(move.To())->GetPieceType()]
    - piece_evaluations_[board.GetPiece(move.From())->GetPieceType()];
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
