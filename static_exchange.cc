#include "static_exchange.h"

#include <algorithm>
#include <iostream>

#include "board.h"


namespace chess {

namespace {

int StaticExchangeEvaluationFromLists(
    int square_piece_eval,
    const std::vector<int>& sorted_piece_values,
    size_t index,
    const std::vector<int>& other_team_sorted_piece_values,
    size_t other_index) {
  if (index >= sorted_piece_values.size()) {
    return 0;
  }
  int value_capture = square_piece_eval - StaticExchangeEvaluationFromLists(
      sorted_piece_values[index],
      other_team_sorted_piece_values,
      other_index,
      sorted_piece_values,
      index + 1);
  return std::max(0, value_capture);
}

int StaticExchangeEvaluationFromLocation(
    int piece_evaluations[6],
      const Board& board, const BoardLocation& loc) {
  constexpr size_t kLimit = 5;
  PlacedPiece attackers_this_side[kLimit];
  PlacedPiece attackers_that_side[kLimit];

  size_t num_attackers_this_side = board.GetAttackers2(
      attackers_this_side, kLimit, board.GetTurn().GetTeam(), loc);
  size_t num_attackers_that_side = board.GetAttackers2(
      attackers_that_side, kLimit, OtherTeam(board.GetTurn().GetTeam()), loc);

  std::vector<int> piece_values_this_side;
  piece_values_this_side.reserve(num_attackers_this_side);
  std::vector<int> piece_values_that_side;
  piece_values_that_side.reserve(num_attackers_that_side);

  for (size_t i = 0; i < num_attackers_this_side; ++i) {
    const auto& placed_piece = attackers_this_side[i];
    int piece_eval = piece_evaluations[placed_piece.GetPiece().GetPieceType()];
    piece_values_this_side.push_back(piece_eval);
  }

  for (size_t i = 0; i < num_attackers_that_side; ++i) {
    const auto& placed_piece = attackers_that_side[i];
    int piece_eval = piece_evaluations[placed_piece.GetPiece().GetPieceType()];
    piece_values_that_side.push_back(piece_eval);
  }

  std::sort(piece_values_this_side.begin(), piece_values_this_side.end());
  std::sort(piece_values_that_side.begin(), piece_values_that_side.end());

  const auto attacking = board.GetPiece(loc);
  assert(attacking.Present());
  int attacked_piece_eval = piece_evaluations[attacking.GetPieceType()];

  return StaticExchangeEvaluationFromLists(
      attacked_piece_eval,
      piece_values_this_side,
      0,
      piece_values_that_side,
      0);
}

} // namespace


int StaticExchangeEvaluationCapture(
    int piece_evaluations[6],
    Board& board,
    const Move& move) {

  const auto captured = move.GetStandardCapture();
  assert(captured.Present());

  int value = piece_evaluations[captured.GetPieceType()];

  board.MakeMove(move);
  value -= StaticExchangeEvaluationFromLocation(piece_evaluations, board, move.To());
  board.UndoMove();
  return value;
}

}  // namespace chess
