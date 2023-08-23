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
    Stats<int, 14, 14, 14, 14>* history_heuristic,
    Stats<int, kNumPieceTypes, 14, 14, kNumPieceTypes>* capture_history,
    const Stats<int, kNumPieceTypes, 14, 14>** continuation_history,
    int piece_move_order_scores[6],
    bool enable_move_order_checks,
    Move* buffer,
    size_t buffer_size,
    const Move& counter_move) {
  enable_move_order_checks_ = enable_move_order_checks;
  stages_.resize(5);
  moves_ = buffer;
  num_moves_ = board.GetPseudoLegalMoves2(buffer, buffer_size);
  board_ = &board;

  for (size_t i = 0; i < num_moves_; i++) {
    const auto& move = moves_[i];

    const auto capture = move.GetCapturePiece();
    const auto& piece = board.GetPiece(move.From());
    const auto piece_type = piece.GetPieceType();
    const auto& from = move.From();
    const auto& to = move.To();
    int to_row = to.GetRow();
    int to_col = to.GetCol();

    int score = piece_move_order_scores[piece_type];
    if (pvmove.has_value() && move == pvmove.value()) {
      stages_[PV_MOVE].emplace_back(i, score);
    } else if (killers[0] == move
            || killers[1] == move
            || counter_move == move
            ) {
      if (move == killers[0]) {
        score += 2;
      } else if (move == killers[1]) {
        score += 1;
      }
      stages_[KILLER].emplace_back(i, score);
    } else if (move.IsCapture()) {
      // We'd ideally use SEE here, if it weren't so expensive to compute.
      PieceType attacker_piece_type = piece_type;
      PieceType captured_piece_type = capture.GetPieceType();
      int captured_val = piece_evaluations[captured_piece_type];
      int attacker_val = piece_evaluations[attacker_piece_type];
      score += captured_val - attacker_val/100;
      if (capture_history != nullptr) {
        score += (*capture_history)[attacker_piece_type][to.GetRow()][to.GetCol()][captured_piece_type];
      }
      if (attacker_val <= captured_val) {
        stages_[GOOD_CAPTURE].emplace_back(i, score);
      } else {
        stages_[BAD_CAPTURE].emplace_back(i, score);
      }
    } else {
      if (history_heuristic != nullptr) {
        score += 2 * (*history_heuristic)[from.GetRow()][from.GetCol()][to.GetRow()][to.GetCol()];
      }
      if (continuation_history != nullptr) {
        score += 2 * (*continuation_history[0])[piece_type][to_row][to_col]
               +     (*continuation_history[1])[piece_type][to_row][to_col]
               +     (*continuation_history[2])[piece_type][to_row][to_col]
               +     (*continuation_history[3])[piece_type][to_row][to_col]
               ;
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
