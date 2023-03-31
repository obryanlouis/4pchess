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

constexpr int kMaxQuiescenceDepth = 4;
constexpr int kMaxQuiescenceMoves = 2;

namespace {

std::vector<Move> PrunedMoveOrder(
    Board& board, bool maximizing_player, bool prune) {

  auto moves = board.GetPseudoLegalMoves();

  std::vector<size_t> checks;
  std::vector<size_t> captures;
  std::vector<std::tuple<size_t, float>> noncapture_pos_and_score;
  Player player = board.GetTurn();
  float max_pl_mult = maximizing_player ? 1 : -1;
  for (size_t i = 0; i < moves.size(); i++) {
    const auto& move = moves[i];

    if (board.DeliversCheck(move)) {
      checks.push_back(i);
    } else if (move.GetStandardCapture() != nullptr
               || move.GetEnpassantCapture() != nullptr) {
      captures.push_back(i);
    } else {
      board.MakeMove(move);
      float score = max_pl_mult * board.MobilityEvaluation(player);
      board.UndoMove();
      noncapture_pos_and_score.push_back(std::make_tuple(i, score));
    }
  }

  std::vector<Move> pruned_moves;

  struct {
    bool operator()(std::pair<size_t, float> a, std::pair<size_t, float> b) {
      return std::get<1>(a) < std::get<1>(b);
    }
  } customLess;

  std::sort(noncapture_pos_and_score.begin(), noncapture_pos_and_score.end(),
            customLess);

  int i = 0;
  constexpr int kNumNoncaptures = 4;
  if (prune) {
    i = std::max((int)noncapture_pos_and_score.size() - kNumNoncaptures, 0);
  }

  for (; i < noncapture_pos_and_score.size() - 1; i++) {
    const auto& entry = noncapture_pos_and_score[i];
    size_t pos = std::get<0>(entry);
    pruned_moves.push_back(moves[pos]);
  }

  for (const auto& pos : captures) {
    pruned_moves.push_back(moves[pos]);
  }

  for (const auto& pos : checks) {
    pruned_moves.push_back(moves[pos]);
  }

  return pruned_moves;
}

}  // namespace


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

  piece_move_order_scores_[PAWN] = 0.1;
  piece_move_order_scores_[KNIGHT] = 0.2;
  piece_move_order_scores_[BISHOP] = 0.3;
  piece_move_order_scores_[ROOK] = 0.4;
  piece_move_order_scores_[QUEEN] = 0.5;
  piece_move_order_scores_[KING] = 0.0;

  if (options_.enable_transposition_table) {
    hash_table_ = (HashTableEntry*) calloc(
        options_.transposition_table_size, sizeof(HashTableEntry));
    assert(
        (hash_table_ != nullptr) && 
        "Can't create transposition table. Try using a smaller size.");
  }

}


// https://www.chessprogramming.org/Quiescence_Search
std::optional<float> AlphaBetaPlayer::QuiescenceSearch(
    Board& board,
    int depth_left,
    float alpha,
    float beta,
    bool maximizing_player,
    const std::optional<
        std::chrono::time_point<std::chrono::system_clock>>& deadline) {
  if (canceled_.load() || (deadline.has_value()
        && std::chrono::system_clock::now() >= deadline.value())) {
    return std::nullopt;
  }

  float stand_pat = Evaluate(board);

  if (!maximizing_player) {
    // Evaluate returns the board value w.r.t. RY team
    stand_pat = -stand_pat;
  }

  if (depth_left <= 0) {
    return stand_pat;
  }

  if (stand_pat >= beta) {
    return beta;
  }
  alpha = std::max(alpha, stand_pat);

  std::vector<Move> pseudo_legal_moves = MoveOrder(board, maximizing_player);

  Player turn = board.GetTurn();
  int turn_i = static_cast<int>(turn.GetColor());
  float curr_mob_score = player_mobility_scores_[turn_i];

  int num_moves_examined = 0;
  bool has_legal_move = false;

  for (int i = pseudo_legal_moves.size() - 1;
      i >= 0 && num_moves_examined < kMaxQuiescenceMoves; i--) {
    const auto& move = pseudo_legal_moves[i];
    board.MakeMove(move);

    if (board.CheckWasLastMoveKingCapture() != IN_PROGRESS) {
      board.UndoMove();
      has_legal_move = true;
      alpha = std::numeric_limits<float>::infinity();
      break;
    }

    if (options_.enable_mobility_evaluation) {
      float player_mobility_score = board.MobilityEvaluation(turn);
      player_mobility_scores_[turn_i] = player_mobility_score;
    }

    if (board.IsKingInCheck(turn)) { // Illegal move
      board.UndoMove();

      continue;
    }

    has_legal_move = true;

    if (move.GetStandardCapture() != nullptr) {
      // quiet move -- skip
      board.UndoMove();
      continue;
    }

    auto value_or = QuiescenceSearch(
        board, depth_left - 1, -beta, -alpha, !maximizing_player,
        deadline);

    board.UndoMove();

    num_moves_examined++;

    if (!value_or.has_value()) {
      return std::nullopt; // timeout
    }

    alpha = std::max(alpha, -value_or.value());
    if (alpha >= beta) {
      break; // cutoff
    }
  }

  if (options_.enable_mobility_evaluation) {
    player_mobility_scores_[turn_i] = curr_mob_score; // reset
  }

  if (!has_legal_move) {
    // no legal moves
    if (!board.IsKingInCheck(turn)) {
      alpha = 0; // stalemate
    }

    return -std::numeric_limits<float>::infinity(); // checkmate
  }

  return alpha;
}


// Nega-max search (basically the same as alpha beta).
// https://www.chessprogramming.org/Alpha-Beta
// Returns (nega-max value, best move) pair.
// The best move is nullopt if the game is over.
// If the function returns std::nullopt, then it hit the deadline
// before finishing search and the results should not be used.
std::optional<std::pair<float, std::optional<Move>>> AlphaBetaPlayer::NegaMax(
    Board& board,
    int depth,
    int depth_left,
    float alpha,
    float beta,
    bool maximizing_player,
    const std::optional<
        std::chrono::time_point<std::chrono::system_clock>>& deadline,
    PVInfo& pvinfo) {
  if (canceled_.load() || (deadline.has_value()
        && std::chrono::system_clock::now() >= deadline.value())) {
    return std::nullopt;
  }

  if (options_.enable_transposition_table) { // return hash table value if any
    int64_t hash = board.HashKey();
    size_t n = hash % options_.transposition_table_size;
    const HashTableEntry& entry = hash_table_[n];
    if (entry.key == hash) { // valid entry
      num_cache_hits_++;
      if (entry.depth >= depth_left) {
        if (entry.bound == EXACT) {
          return std::make_pair(entry.score, entry.best_move);
        }
        if (maximizing_player) {
          if (entry.bound == LOWER_BOUND) {
            if (entry.score >= beta) {
              return std::make_pair(entry.score, entry.best_move);
            }
          } else {
            beta = std::min(beta, entry.score);
          }
        } else {
          if (entry.bound == UPPER_BOUND) {
            if (entry.score <= alpha) {
              return std::make_pair(entry.score, entry.best_move);
            }
          } else {
            alpha = std::max(alpha, entry.score);
          }
        }
      }
    }
  }

  if (depth_left <= 0) {

    if (options_.enable_quiescence) {
      auto value_or = QuiescenceSearch(
          board, kMaxQuiescenceDepth, alpha, beta, maximizing_player, deadline);
      if (!value_or.has_value()) {
        return std::nullopt; // timeout
      }
      return std::make_pair(value_or.value(), std::nullopt);
    }

    float eval = Evaluate(board);
    if (!maximizing_player) {
      // Evaluate returns the board value w.r.t. RY team
      eval = -eval;
    }

    if (options_.enable_static_exchange) {
      if (board.LastMoveWasCapture()) {
        const Move& move = board.GetLastMove();
        board.UndoMove();
        eval -= StaticExchangeEvaluation(board, move); // sub since move undone
        board.MakeMove(move);
      }
    }

    return std::make_pair(eval, std::nullopt);
  }

  std::vector<Move> pseudo_legal_moves;

  pseudo_legal_moves = PrunedMoveOrder(board, maximizing_player, true);
//  if (options_.test) {
//    pseudo_legal_moves = PrunedMoveOrder(board, maximizing_player, depth >= 2);
//  } else {
//    pseudo_legal_moves = MoveOrder(board, maximizing_player);
//  }


  std::optional<Move> pv_move = pvinfo.GetBestMove();
  if (options_.enable_principal_variation && pv_move.has_value()) {
    pseudo_legal_moves.push_back(pv_move.value());
  }

  float value = -std::numeric_limits<float>::infinity();
  std::optional<Move> best_move;
  Player turn = board.GetTurn();
  int turn_i = static_cast<int>(turn.GetColor());
  float curr_mob_score = player_mobility_scores_[turn_i];
  bool cutoff = false;
  for (int i = pseudo_legal_moves.size() - 1; i >= 0; i--) {
    const auto& move = pseudo_legal_moves[i];

    board.MakeMove(move);

    if (board.CheckWasLastMoveKingCapture() != IN_PROGRESS) {
      board.UndoMove();
      value = std::numeric_limits<float>::infinity();
      best_move = move;
      pvinfo.SetBestMove(move);
      break;
    }

    if (options_.enable_mobility_evaluation) {
      float player_mobility_score = board.MobilityEvaluation(turn);
      player_mobility_scores_[turn_i] = player_mobility_score;
    }

    if (board.IsKingInCheck(turn)) {
      board.UndoMove();
      if (options_.enable_mobility_evaluation) {
        player_mobility_scores_[turn_i] = curr_mob_score;
      }

      continue;
    }

    bool is_pv_move = pv_move.has_value() && pv_move.value() == move;

    std::shared_ptr<PVInfo> child_pvinfo;
    if (is_pv_move && pvinfo.GetChild() != nullptr) {
      child_pvinfo = pvinfo.GetChild();
    } else {
      child_pvinfo = std::make_shared<PVInfo>();
    }

    int new_depth_left = depth_left - 1;
    bool lmr = options_.enable_late_move_reduction
//            && depth >= 4
            && !is_pv_move
            && move.GetStandardCapture() == nullptr
            && move.GetEnpassantCapture() == nullptr
            // note: also add checks here, when we can compute them cheaply
    ;
    if (lmr) {
      new_depth_left = std::max(0, new_depth_left - 1);
    }

    auto value_and_move = NegaMax(
        board, depth + 1, new_depth_left, -beta, -alpha, !maximizing_player,
        deadline, *child_pvinfo);

    board.UndoMove();

    if (lmr
        && value_and_move.has_value()
        && std::get<0>(value_and_move.value()) > alpha) {
      // Did not fail low -- redo search
      value_and_move = NegaMax(
          board, depth + 1, depth_left - 1, -beta, -alpha, !maximizing_player,
          deadline, *child_pvinfo);
    }

    if (options_.enable_mobility_evaluation) {
      player_mobility_scores_[turn_i] = curr_mob_score;
    }

    if (!value_and_move.has_value()) {
      return std::nullopt; // timeout
    }
    float val = -std::get<0>(value_and_move.value());
    if (val > value || !best_move.has_value()) {
      value = val;
      best_move = move;
      // If PV move changed, update it
      if (!pv_move.has_value() || pv_move.value() != move) {
        pvinfo.SetBestMove(move);
        pvinfo.SetChild(child_pvinfo);
        pv_move = best_move;
      }
    }
    if (value > beta) {
      cutoff = true;
      break; // cutoff
    }
    if (value > alpha) {
      alpha = value;
    }
  }

  if (options_.enable_transposition_table) {
    int64_t hash = board.HashKey();
    size_t n = hash % options_.transposition_table_size;
    HashTableEntry& entry = hash_table_[n];
    entry.key = hash;
    entry.depth = depth;
    entry.best_move = best_move;
    entry.score = maximizing_player ? value : -value;
    if (cutoff) {
      entry.bound = maximizing_player ? LOWER_BOUND : UPPER_BOUND;
    } else {
      entry.bound = EXACT;
    }
  }

  if (!best_move.has_value()) {
    // no legal moves
    if (!board.IsKingInCheck(board.GetTurn())) {
      value = 0;
    }

    return std::make_pair(value, std::nullopt);
  }

  return std::make_pair(value, best_move);
}


float AlphaBetaPlayer::Evaluate(Board& board) {
  num_evaluations_++;

  GameResult game_result = board.CheckWasLastMoveKingCapture();
  // Base case: game is over
  if (game_result != IN_PROGRESS) {
    if (game_result == WIN_RY) {
      return std::numeric_limits<float>::infinity();
    }
    if (game_result == WIN_BG) {
      return -std::numeric_limits<float>::infinity();
    }
    // stalemate
    return 0;
  }

  float score = board.PieceEvaluation();
  if (options_.enable_mobility_evaluation) {
    for (int i = 0; i < 4; i++) {
      score += player_mobility_scores_[i];
    }
    //score += board.MobilityEvaluation();
  }
  return score;
}

std::optional<std::tuple<float, std::optional<Move>, int>> AlphaBetaPlayer::MakeMove(
    Board& board,
    std::optional<std::chrono::milliseconds> time_limit,
    int max_depth) {
  // Use Alpha-Beta search with iterative deepening

  std::optional<std::chrono::time_point<std::chrono::system_clock>> deadline;
  auto start = std::chrono::system_clock::now();
  if (time_limit.has_value()) {
    deadline = start + time_limit.value();
  }

  if (options_.enable_mobility_evaluation) {
    for (int i = 0; i < 4; i++) {
      Player player(static_cast<PlayerColor>(i));
      player_mobility_scores_[i] = board.MobilityEvaluation(player);
    }
  }

  int next_depth = 1 + pv_info_.GetDepth();
  std::optional<std::pair<float, std::optional<Move>>> res;
  float inf = std::numeric_limits<float>::infinity();
  float alpha = -inf;
  float beta = inf;
  bool maximizing_player = board.TeamToPlay() == RED_YELLOW;
  int searched_depth = 0;

  while (next_depth <= max_depth) {
    auto move_and_value = NegaMax(
        board, 1, next_depth, alpha, beta, maximizing_player,
        deadline, pv_info_);

    if (!move_and_value.has_value()) { // Hit deadline
      break;
    }
    res = move_and_value;
    searched_depth = next_depth;
    next_depth++;
    float evaluation = std::get<0>(move_and_value.value());
    if (std::isinf(evaluation)) {
      break;  // Proven win/loss
    }
  }

  if (res.has_value()) {
    float eval = std::get<0>(res.value());
    if (!maximizing_player) {
      eval = -eval;
    }
    return std::make_tuple(eval, std::get<1>(res.value()), searched_depth);
  }

  return std::nullopt;
}

std::vector<Move> AlphaBetaPlayer::MoveOrder(
    Board& board, bool maximizing_player) {
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
  std::vector<std::pair<size_t, float>> pos_and_score;
  pos_and_score.reserve(moves.size());
//  float max_pl_mult = maximizing_player ? 1 : -1;
  for (size_t i = 0; i < moves.size(); i++) {
    const auto& move = moves[i];
    float score = 0;

    const auto* capture = move.GetStandardCapture();
    if (capture != nullptr) {
//      score += max_pl_mult * StaticExchangeEvaluation(board, move);
      score += piece_evaluations_[static_cast<int>(capture->GetPieceType())];
    } else {
//      const auto* piece = board.GetPiece(move.From());
//      score += piece_move_order_scores_[piece->GetPieceType()];
    }

    const auto* piece = board.GetPiece(move.From());
    score += piece_move_order_scores_[static_cast<int>(piece->GetPieceType())];

//    const auto& promotion_piece_type = move.GetPromotionPieceType();
//    if (promotion_piece_type.has_value()) {
//      score += piece_evaluations_[
//        static_cast<int>(promotion_piece_type.value())];
//    }

    pos_and_score.push_back(std::make_pair(i, score));
  }

  struct {
    bool operator()(std::pair<size_t, float> a, std::pair<size_t, float> b) {
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

