#ifndef _MOVE_PICKER_H_
#define _MOVE_PICKER_H_

#include <array>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <optional>
#include <tuple>
#include <type_traits>  // IWYU pragma: keep

#include "board.h"


namespace chess {

////////////////////////////////////////////////////////////////////////////////
/// Stats
////////////////////////////////////////////////////////////////////////////////

// StatsEntry stores the stat table value. It is usually a number but could
// be a move or even a nested history. We use a class instead of a naked value
// to directly call history update operator<<() on the entry so to use stats
// tables at caller sites as simple multi-dim arrays.
template<typename T, int D>
class StatsEntry {

    T entry;

   public:
    void operator=(const T& v) { entry = v; }
    T*   operator&() { return &entry; }
    T*   operator->() { return &entry; }
    operator const T&() const { return entry; }

    void operator<<(int bonus) {
        assert(abs(bonus) <= D);  // Ensure range is [-D, D]
        static_assert(D <= std::numeric_limits<T>::max(), "D overflows T");

        //entry += bonus - entry * abs(bonus) / D;
        entry += std::min(D - entry, bonus);

        assert(abs(entry) <= D);
    }
};

// Stats is a generic N-dimensional array used to store various statistics.
// The first template parameter T is the base type of the array, and the second
// template parameter D limits the range of updates in [-D, D] when we update
// values with the << operator, while the last parameters (Size and Sizes)
// encode the dimensions of the array.
template<typename T, int D, int Size, int... Sizes>
struct Stats: public std::array<Stats<T, D, Sizes...>, Size> {
    using stats = Stats<T, D, Size, Sizes...>;

    void fill(const T& v) {

        // For standard-layout 'this' points to the first struct member
        assert(std::is_standard_layout_v<stats>);

        using entry = StatsEntry<T, D>;
        entry* p    = reinterpret_cast<entry*>(this);
        std::fill(p, p + sizeof(*this) / sizeof(entry), v);
    }
};

template<typename T, int D, int Size>
struct Stats<T, D, Size>: public std::array<StatsEntry<T, D>, Size> {};

// In stats table, D=0 means that the template parameter is not used
enum StatsParams {
    NOT_USED = 0
};
enum StatsType {
    NoCaptures,
    Captures
};

// Addressed by [piece][to]
using PieceToHistory = Stats<int32_t, 2147483647, 7, 14, 14>;

// Addressed by [piece_1][to_1][piece_2][to_2]
using ContinuationHistory = Stats<PieceToHistory, NOT_USED, 7, 14, 14>;

////////////////////////////////////////////////////////////////////////////////

class MovePicker {
 public:
  MovePicker(
    Board& board,
    const std::optional<Move>& pvmove,
    Move* killers,
    const int piece_evaluations[6],
    int history_heuristic[6][14][14][14][14],
    int capture_heuristic[6][4][6][4][14][14],
    int piece_move_order_scores[6],
    bool enable_move_order_checks,
    Move* buffer,
    size_t buffer_size
    ,Move* counter_moves
    ,bool include_quiets = true
    ,const PieceToHistory** piece_to_history = nullptr
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

