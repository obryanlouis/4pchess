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
#include "move_picker.h"

namespace chess {

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

  king_attacker_values_[PAWN] = 0;
  king_attacker_values_[KNIGHT] = 60;
  king_attacker_values_[BISHOP] = 80;
  king_attacker_values_[ROOK] = 100;
  king_attacker_values_[QUEEN] = 160;
  king_attacker_values_[KING] = 0;

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

  move_buffer_ = new Move[buffer_partition_size_ * buffer_num_partitions_];
}

AlphaBetaPlayer::~AlphaBetaPlayer() {
  delete[] move_buffer_;
}

Move* AlphaBetaPlayer::GetNextMoveBufferPartition() {
  if (buffer_id_ >= buffer_num_partitions_) {
    std::cout << "AlphaBetaPlayer move buffer overflow" << std::endl;
    abort();
  }
  return &move_buffer_[buffer_id_++ * buffer_partition_size_];
}

void AlphaBetaPlayer::ReleaseMoveBufferPartition() {
  assert(buffer_id_ > 0);
  buffer_id_--;
}

int AlphaBetaPlayer::Reduction(int depth, int move_number) const {
  return (reductions_[depth] * reductions_[move_number] + 1500) / 1000;
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
  depth = std::max(depth, 0);
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
  Player player = board.GetTurn();

  int eval = Evaluate(board, maximizing_player, alpha, beta);

  //Team team = player.GetTeam();

  if (depth <= 0 || ply >= kMaxPly) {

    if (options_.enable_transposition_table) {
      transposition_table_->Save(board.HashKey(), 0, std::nullopt, eval, EXACT, is_pv_node);
    }

    return std::make_tuple(eval, std::nullopt);
  }

  (ss+2)->killers[0] = (ss+2)->killers[1] = Move();
  ss->move_count = 0;

  bool in_check = board.IsKingInCheck(player);
  bool partner_checked = board.IsKingInCheck(GetPartner(player));

//  bool any_player_checked = in_check || partner_checked;
//  if (!any_player_checked) {
//    // Note that the last player is not in check
//    PlayerColor color = static_cast<PlayerColor>((player.GetColor() + 1) % 4);
//    Player other_player(color);
//    any_player_checked = board.IsKingInCheck(other_player);
//  }

  // null move pruning
  if (options_.enable_null_move_pruning
      && !is_root_node // not root
      && !is_pv_node // not a pv node
      && null_moves == 0 // last move wasn't null
      && !in_check // not in check
      && eval >= beta
      && !partner_checked
      ) {
    num_null_moves_tried_++;
    board.MakeNullMove();

    // try the null move with possibly reduced depth
    PVInfo null_pvinfo;
    int r = std::min(depth / 3 + 1, depth - 1);
    auto value_and_move_or = Search(
        ss+1, NonPV, board, ply + 1, depth - r,
        -beta, -beta + 1, !maximizing_player, expanded, deadline, null_pvinfo,
        null_moves + 1, !isCutNode);

    board.UndoNullMove();

    // if it failed high, skip this move
    if (value_and_move_or.has_value()
        && -std::get<0>(value_and_move_or.value()) >= beta) {
      num_null_moves_pruned_++;
      
      // reset killers
	    (ss+1)->killers[0] = {};
	    (ss+1)->killers[1] = {};

      return std::make_tuple(beta, std::nullopt);
    }
  }

  std::optional<Move> best_move;
  int player_color = static_cast<int>(player.GetColor());
  int curr_mob_score = player_mobility_scores_[player_color];

  std::optional<Move> pv_move = pvinfo.GetBestMove();
  Move* moves = GetNextMoveBufferPartition();
  MovePicker move_picker(
    board,
    pv_move,
    ss->killers,
    piece_evaluations_,
    history_heuristic_,
    piece_move_order_scores_,
    options_.enable_move_order_checks,
    moves,
    buffer_partition_size_);

  bool has_legal_moves = false;
  int move_count = 0;
  int quiets = 0;

  while (true) {
    Move* move_ptr = move_picker.GetNextMove();
    if (move_ptr == nullptr) {
      break;
    }
    Move& move = *move_ptr;

    std::optional<std::tuple<int, std::optional<Move>>> value_and_move_or;

    bool delivers_check = false;

    bool lmr_cond1 =
      options_.enable_late_move_reduction
      && depth > 1
      // (this check is done before move_count is incremented, so use the next)
      && (move_count+1) > 1 + (is_pv_node && ply <= 1)
      && (!is_tt_pv
          || !move.IsCapture()
          || (isCutNode && (ss-1)->move_count > 1))
      && !in_check
      && !partner_checked
         ;

    if (lmr_cond1 || options_.enable_late_move_pruning) {
      // this has to be called before the move is made
      delivers_check = move.DeliversCheck(board);
    }

    bool quiet = !in_check && !move.IsCapture() && !delivers_check
      ;

    if (options_.enable_late_move_pruning
        && quiet
        && !is_tt_pv
        && !is_pv_node
        && quiets >= 1+depth*depth/5
        ) {
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

    ss->move_count = move_count++;
    if (quiet) {
      quiets++;
    }

    if (options_.enable_mobility_evaluation) {
      player_mobility_scores_[player_color] = board.MobilityEvaluation(player);
    }

    bool is_pv_move = pv_move.has_value() && pv_move.value() == move;

    std::shared_ptr<PVInfo> child_pvinfo;
    if (is_pv_move && pvinfo.GetChild() != nullptr) {
      child_pvinfo = pvinfo.GetChild();
    } else {
      child_pvinfo = std::make_shared<PVInfo>();
    }

    int e = 0;  // extension

    // check extensions at early moves.
    if (options_.enable_check_extensions
        && in_check
        && move_count < 6
        && expanded < 3) {
      num_check_extensions_++;
      e = 1;
    }

    bool lmr = lmr_cond1
        && !delivers_check;

    if (lmr) {
      num_lmr_searches_++;

      int r = Reduction(depth, move_count + 1);
      r = std::clamp(r, 0, depth - 2);
      value_and_move_or = Search(
          ss+1, NonPV, board, ply + 1, depth - 1 - r + e,
          -(alpha + 1), -alpha, !maximizing_player, expanded + e,
          deadline, *child_pvinfo, null_moves, true);
      if (value_and_move_or.has_value()) {
        int score = -std::get<0>(value_and_move_or.value());
        if (score > alpha) {  // re-search
          num_lmr_researches_++;
          value_and_move_or = Search(
              ss+1, NonPV, board, ply + 1, depth - 1 + e,
              -(alpha + 1), -alpha, !maximizing_player, expanded + e,
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
            deadline, *child_pvinfo, null_moves, false);
    }

    board.UndoMove();

    if (options_.enable_mobility_evaluation) { // reset
      player_mobility_scores_[player_color] = curr_mob_score;
    }

    if (!value_and_move_or.has_value()) {
      ReleaseMoveBufferPartition();
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
      pvinfo.SetChild(child_pvinfo);
      pvinfo.SetBestMove(move);
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

  ReleaseMoveBufferPartition();
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

//constexpr int kPieceImbalanceTable[16] = {
//  0, -25, -45, -100, -135, -200, -700, -800,
//  -800, -800, -800, -800, -800, -800, -800, -800,
//};

constexpr int kPieceImbalanceTable[16] = {
  0, -50, -100, -300, -600, -700, -800, -800,
  -800, -800, -800, -800, -800, -800, -800, -800,
};

std::tuple<int, int> GetPieceStatistics(
    const std::vector<PlacedPiece>& pieces,
    PlayerColor color) {
  int num_major = 0;
  float sum_center_dist = 0;
  for (const auto& placed_piece : pieces) {
    PieceType pt = placed_piece.GetPiece().GetPieceType();
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

int AlphaBetaPlayer::Evaluate(Board& board, bool maximizing_player, int alpha, int beta) {
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

//    // DEBUG: just use piece eval
//    return maximizing_player ? eval : -eval;

    // Mobility evaluation
    if (options_.enable_mobility_evaluation) {
      for (int i = 0; i < 4; i++) {
        eval += player_mobility_scores_[i];
      }
    }

    auto lazy_skip = [&](int margin) {
      if (!options_.enable_lazy_eval) {
        return false;
      }
      int re = maximizing_player ? eval : -eval; // returned eval
      return re + margin <= alpha || re >= beta + margin;
    };

    int piece_development_margin = 400;
    int king_safety_margin = 600;

    if (lazy_skip(piece_development_margin + king_safety_margin)) {
      num_lazy_eval_++;
      return maximizing_player ? eval : -eval;
    }

    if (options_.enable_piece_imbalance
        || options_.enable_piece_development) {
      const auto& piece_list = board.GetPieceList();
      auto red_stats = GetPieceStatistics(piece_list[RED], RED);
      auto blue_stats = GetPieceStatistics(piece_list[BLUE], BLUE);
      auto yellow_stats = GetPieceStatistics(piece_list[YELLOW], YELLOW);
      auto green_stats = GetPieceStatistics(piece_list[GREEN], GREEN);

      // Piece imbalance
      if (options_.enable_piece_imbalance) {
        eval -= kPieceImbalanceTable[std::abs(std::get<0>(red_stats) - std::get<0>(yellow_stats))];
        eval += kPieceImbalanceTable[std::abs(std::get<0>(blue_stats) - std::get<0>(green_stats))];
      }

      // Piece development
      if (options_.enable_piece_development) {
        eval -= 50 * (std::get<1>(red_stats) + std::get<1>(yellow_stats)
          - std::get<1>(blue_stats) - std::get<1>(green_stats));
      }

    }

    if (lazy_skip(king_safety_margin)) {
      num_lazy_eval_++;
      return maximizing_player ? eval : -eval;
    }

    // King safety evaluation (no lazy eval)
    if (options_.enable_king_safety) {
      for (int color = 0; color < 4; ++color) {
        int king_safety = 0;
        PlayerColor pl_cl = static_cast<PlayerColor>(color);
        Player player(pl_cl);
        Team team = player.GetTeam();
        Team other = OtherTeam(team);
        const auto king_location = board.GetKingLocation(pl_cl);
        if (king_location.Present()) {

          if (options_.enable_pawn_shield) {
            bool shield = HasShield(board, pl_cl, king_location);
            bool on_back_rank = OnBackRank(king_location);
            if (!shield) {
              king_safety -= 75;
            }
            if (!on_back_rank) {
              king_safety -= 50;
            }
            if (!shield && !on_back_rank) {
              king_safety -= 50;
            }
          }

          if (options_.enable_attacking_king_zone) {

            for (int delta_row = -1; delta_row <= 1; ++delta_row) {
              for (int delta_col = -1; delta_col <= 1; ++delta_col) {
                int row = king_location.GetRow() + delta_row;
                int col = king_location.GetCol() + delta_col;
                BoardLocation loc(row, col);
                if (!board.IsLegalLocation(loc) || OnBackRank(loc)) {
                  continue;
                }
                BoardLocation piece_location(row, col);

                PlacedPiece attackers[5];
                size_t num_attackers = board.GetAttackers2(attackers, 5, other, piece_location);

                if (num_attackers > 0) {
                  int value_of_attacks = 0;
                  int attacker_colors[4] = {0, 0, 0, 0};
                  for (size_t attacker_id = 0; attacker_id < num_attackers; attacker_id++) {
                    const auto& placed_piece = attackers[attacker_id];
                    const auto& piece = placed_piece.GetPiece();
                    int val = king_attacker_values_[piece.GetPieceType()];
                    value_of_attacks += val;
                    if (val > 0) {
                      attacker_colors[piece.GetColor()]++;
                    }
                  }
                  int num_attacker_colors = 0;
                  for (int i = 0; i < 4; i++) {
                    if (attacker_colors[i] > 0) {
                      num_attacker_colors++;
                    }
                  }
                  int attack_zone = value_of_attacks * king_attack_weight_[num_attackers] / 100;
                  if (num_attacker_colors > 1) {
                    attack_zone += 50;
                  }
                  king_safety -= attack_zone;
                }
              }
            }
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
  // w.r.t. maximizing team
  return maximizing_player ? eval : -eval;
}

void AlphaBetaPlayer::ResetHistoryHeuristic() {
  std::memset(history_heuristic_, 0, (14*14*14*14) * sizeof(int) / sizeof(char));
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

    move_and_value = Search(
        ss, Root, board, 1, next_depth, alpha, beta, maximizing_player, 0,
        deadline, pv_info_);

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

//int AlphaBetaPlayer::StaticExchangeEvaluationCapture(
//    Board& board,
//    const Move& move) const {
//
//  const auto captured = move.GetStandardCapture();
//  assert(captured.Present());
//
//  int value = piece_evaluations_[captured.GetPieceType()];
//
//  board.MakeMove(move);
//  value -= StaticExchangeEvaluation(board, move.To());
//  board.UndoMove();
//  return value;
//}
//
//int AlphaBetaPlayer::StaticExchangeEvaluation(
//      const Board& board, const BoardLocation& loc) const {
//  std::vector<PlacedPiece> attackers_this_side = board.GetAttackers(
//      board.GetTurn().GetTeam(), loc);
//  std::vector<PlacedPiece> attackers_that_side = board.GetAttackers(
//      OtherTeam(board.GetTurn().GetTeam()), loc);
//
//  std::vector<int> piece_values_this_side;
//  piece_values_this_side.reserve(attackers_this_side.size());
//  std::vector<int> piece_values_that_side;
//  piece_values_that_side.reserve(attackers_that_side.size());
//
//  for (const auto& placed_piece : attackers_this_side) {
//    int piece_eval = piece_evaluations_[placed_piece.GetPiece().GetPieceType()];
//    piece_values_this_side.push_back(piece_eval);
//  }
//
//  for (const auto& placed_piece : attackers_that_side) {
//    int piece_eval = piece_evaluations_[placed_piece.GetPiece().GetPieceType()];
//    piece_values_that_side.push_back(piece_eval);
//  }
//
//  std::sort(piece_values_this_side.begin(), piece_values_this_side.end());
//  std::sort(piece_values_that_side.begin(), piece_values_that_side.end());
//
//  const auto attacking = board.GetPiece(loc);
//  assert(attacking.Present());
//  int attacked_piece_eval = piece_evaluations_[attacking.GetPieceType()];
//
//  return StaticExchangeEvaluation(
//      attacked_piece_eval,
//      piece_values_this_side,
//      0,
//      piece_values_that_side,
//      0);
//}
//
//int AlphaBetaPlayer::StaticExchangeEvaluation(
//    int square_piece_eval,
//    const std::vector<int>& sorted_piece_values,
//    size_t index,
//    const std::vector<int>& other_team_sorted_piece_values,
//    size_t other_index) const {
//  if (index >= sorted_piece_values.size()) {
//    return 0;
//  }
//  int value_capture = square_piece_eval - StaticExchangeEvaluation(
//      sorted_piece_values[index],
//      other_team_sorted_piece_values,
//      other_index,
//      sorted_piece_values,
//      index + 1);
//  return std::max(0, value_capture);
//}
//
//int AlphaBetaPlayer::ApproxSEECapture(
//    Board& board, const Move& move) const {
//  return piece_evaluations_[board.GetPiece(move.To()).GetPieceType()]
//    - piece_evaluations_[board.GetPiece(move.From()).GetPieceType()];
//}

int PVInfo::GetDepth() const {
  if (best_move_.has_value()) {
    if (child_ == nullptr) {
      return 1;
    }
    return 1 + child_->GetDepth();
  }
  return 0;
}

//int AlphaBetaPlayer::MobilityEvaluation(
//    Board& board, Player player) {
//  Player turn = board.GetTurn();
//  board.SetPlayer(player);
//  int mobility = 0;
//
//  //auto moves = board.GetPseudoLegalMoves();
//  Move* moves = GetNextMoveBufferPartition();
//  size_t num_moves = board.GetPseudoLegalMoves2(moves, buffer_partition_size_);
//
//
//  int piece_mobility[6] = {0, 0, 0, 0, 0, 0};
//  //for (const auto& move : moves) {
//  for (size_t i = 0; i < num_moves; i++) {
//    const auto& move = moves[i];
//    const auto piece = board.GetPiece(move.From());
//    piece_mobility[piece.GetPieceType()]++;
//  }
//  int player_mobility = 0;
//  for (int pt = 0; pt < 6; pt++) {
//    switch (static_cast<PieceType>(pt)) {
//    case QUEEN:
//      player_mobility += std::min(piece_mobility[pt], 12);
//      break;
//    case BISHOP:
//      player_mobility += std::min(piece_mobility[pt], 20);
//      break;
//    case ROOK:
//      player_mobility += std::min(piece_mobility[pt], 20);
//      break;
//    default:
//      player_mobility += piece_mobility[pt];
//      break;
//    }
//  }
//
//  if (player.GetTeam() == RED_YELLOW) {
//    mobility += player_mobility;
//  } else {
//    mobility -= player_mobility;
//  }
//
//  constexpr int kMobilityMultiplier = 5;
//  mobility *= kMobilityMultiplier;
//
//  board.SetPlayer(turn);
//  return mobility;
//}

bool AlphaBetaPlayer::OnBackRank(
    const BoardLocation& loc) {
  return loc.GetRow() == 0 || loc.GetRow() == 13 || loc.GetCol() == 0
    || loc.GetCol() == 13;
}

bool AlphaBetaPlayer::HasShield(
    Board& board, PlayerColor color, const BoardLocation& king_loc) {
  int row = king_loc.GetRow();
  int col = king_loc.GetCol();

  auto ray_blocked = [&](int delta_row, int delta_col) {
    for (int i = 0; i < 2; i++) {
      BoardLocation loc(row + delta_row * (i + 1), col + delta_col * (i + 1));
      if (!board.IsLegalLocation(loc)) {
        return true;
      }
      const auto piece = board.GetPiece(loc);
      if (piece.Present()
          && piece.GetColor() == color) {
        return true;
      }
    }
    return false;
  };

  bool has_shield = true;
  switch (color) {
  case RED:
    return ray_blocked(-1, -1) && ray_blocked(-1, 0) && ray_blocked(-1, 1);
  case BLUE:
    return ray_blocked(-1, 1) && ray_blocked(0, 1) && ray_blocked(1, 1);
  case YELLOW:
    return ray_blocked(1, -1) && ray_blocked(1, 0) && ray_blocked(1, 1);
  case GREEN:
    return ray_blocked(-1, -1) && ray_blocked(0, -1) && ray_blocked(1, -1);
  default:
    abort();
  }
  return has_shield;
}

}  // namespace chess
