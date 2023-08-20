#ifndef _MOVE_PICKER_H_
#define _MOVE_PICKER_H_

#include <optional>
#include <tuple>

#include "board.h"


namespace chess {

class MovePicker {
 public:
  MovePicker(
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
    );

  // If this returns nullptr then there are no more moves
  Move* GetNextMove();
  int GetNumMoves() const { return num_moves_; };

 private:
  struct Item {
    unsigned short index;
    float score;

    Item(short idx, float sco) : index(idx), score(sco) { }
  };

  Board* board_ = nullptr;
  Move* moves_ = nullptr;
  size_t num_moves_ = 0;
  uint8_t stage_ = 0;
  uint8_t stage_idx_ = 0;
  std::vector<std::vector<Item>> stages_;
  bool init_stages_[5] = {false, false, false, false, false};
  bool enable_move_order_checks_;
};

}  // namespace chess

#endif  // _MOVE_PICKER_H_

