#include "move_picker.h"

#include <algorithm>
#include <iostream>

namespace chess {

enum Stage {
  PV_MOVE = 0,
  GOOD_CAPTURE = 1,
  KILLER = 2,
  BAD_CAPTURE = 3,
  QUIET = 4,
};

MovePicker::MovePicker(
    Board& board,
    const std::optional<Move>& pvmove,
    Move* killers,
    int piece_evaluations[6],
    int history_heuristic[14][14][14][14],
    int piece_move_order_scores[6],
    bool enable_move_order_checks,
    Move* buffer,
    size_t buffer_size
    ,Move* counter_moves
    ) {
  enable_move_order_checks_ = enable_move_order_checks;
  stages_.resize(5);
  moves_ = buffer;
  num_moves_ = board.GetPseudoLegalMoves2(buffer, buffer_size);
  board_ = &board;

  for (size_t i = 0; i < num_moves_; i++) {
    const auto& move = moves_[i];

    const auto capture = move.GetCapturePiece();
    const auto piece = board.GetPiece(move.From());

    int score = piece_move_order_scores[piece.GetPieceType()];
    if (pvmove.has_value() && move == pvmove.value()) {
      stages_[PV_MOVE].emplace_back(i, score);
    } else if (killers != nullptr
               && (killers[0] == move || killers[1] == move)) {
      stages_[KILLER].emplace_back(i, score + (move == killers[0] ? 1 : 0));
    } else if (move.IsCapture()) {
      // We'd ideally use SEE here, if it weren't so expensive to compute.
      int captured_val = piece_evaluations[capture.GetPieceType()];
      int attacker_val = piece_evaluations[piece.GetPieceType()];
      score += captured_val - attacker_val/100;
      if (attacker_val <= captured_val) {
        stages_[GOOD_CAPTURE].emplace_back(i, score);
      } else {
        stages_[BAD_CAPTURE].emplace_back(i, score);
      }
    } else {
      const auto& from = move.From();
      const auto& to = move.To();
      score += history_heuristic[from.GetRow()][from.GetCol()][to.GetRow()][to.GetCol()];
      if (move == counter_moves[from.GetRow()*14*14*14 + from.GetCol()*14*14
          + to.GetRow()*14 + to.GetCol()]) {
        score += 50;
      }

      stages_[QUIET].emplace_back(i, score);
    }
  }

}

Move* MovePicker::GetNextMove() {
  // Increment stage_ and stage_idx_ until we find the next item
  while (stage_ < stages_.size() && stage_idx_ >= stages_[stage_].size()) {
    stage_++;
    stage_idx_ = 0;
  }
  if (stage_ >= stages_.size()) {
    return nullptr;
  }

  // Init the stage if not already, including scoring and sorting
  auto& stage_vec = stages_[stage_];
  if (!init_stages_[stage_]) {
    if (stage_vec.size() > 1) {
      if (enable_move_order_checks_) {
        for (auto& item : stage_vec) {
          if (moves_[item.index].DeliversCheck(*board_)) {
            item.score += 10'00;
          }
        }
      }

      struct {
        bool operator()(const Item& a, const Item& b) {
          return a.score > b.score;
        }
      } customLess;

      std::sort(stage_vec.begin(), stage_vec.end(), customLess);
    }

    init_stages_[stage_] = true;
  }

  Move* move = &moves_[stage_vec[stage_idx_].index];

  // Increment stage_idx_ for the next call.
  stage_idx_++;

  return move;
}

}  // namespace chess
