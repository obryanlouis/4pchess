#ifndef _PLAYER_H_
#define _PLAYER_H_

#include <atomic>
#include <chrono>
#include <memory>
#include <optional>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "board.h"

namespace chess {

class PVInfo {
 public:
  PVInfo() = default;

  const std::optional<Move>& GetBestMove() const { return best_move_; }
  std::shared_ptr<PVInfo> GetChild() const { return child_; }
  void SetBestMove(Move move) { best_move_ = std::move(move); }
  void SetChild(std::shared_ptr<PVInfo> child) { child_ = std::move(child); }
  int GetDepth() const;

 private:
  std::optional<Move> best_move_ = std::nullopt;
  std::shared_ptr<PVInfo> child_ = nullptr;
};

constexpr size_t kTranspositionTableSize = 10'000'000;
constexpr int kMaxMoves = 300;

struct PlayerOptions {
  bool enable_move_order = true;
  bool enable_move_order_checks = true;
  bool enable_history_heuristic = true;
  bool enable_mobility_evaluation = true;
  bool pvs = true;
  bool enable_null_move_pruning = true;
  bool enable_late_move_reduction = true;

  // generic test change
  bool test = false;

  bool enable_static_exchange = false;
  bool enable_transposition_table = false; // low hit rate, ineffective
  bool enable_quiescence = false;
  bool enable_history_leaf_pruning = false;
  bool enable_futility_pruning = false;
  bool enable_extensions = false;
  bool enable_mtdf = false;  // performs worse?

  size_t transposition_table_size = kTranspositionTableSize;
  std::optional<int> max_search_depth;
};

enum ScoreBound {
  EXACT = 0, LOWER_BOUND = 1, UPPER_BOUND = 2,
};

struct HashTableEntry {
  int64_t key;
  int depth;
  std::optional<Move> move;
  int score;
  ScoreBound bound;
};

class AlphaBetaPlayer {
 public:
  AlphaBetaPlayer(std::optional<PlayerOptions> options = std::nullopt);
  std::optional<std::tuple<int, std::optional<Move>, int>> MakeMove(
      Board& board,
      std::optional<std::chrono::milliseconds> time_limit = std::nullopt,
      int max_depth = 20);
  int Evaluate(Board& board, bool maximizing_player);
  std::vector<Move> MoveOrder(
      Board& board, bool maximizing_player, const std::optional<Move>& pvmove);
  void CancelEvaluation() { canceled_ = true; }
  // NOTE: Should wait until evaluation is done before resetting this to true.
  void SetCanceled(bool canceled) { canceled_ = canceled; }
  bool IsCanceled() { return canceled_.load(); }
  const PVInfo& GetPVInfo() const { return pv_info_; }

  std::optional<std::tuple<int, std::optional<Move>>> Search(
      Board& board,
      int depth,
      int depth_left,
      int alpha,
      int beta,
      bool maximizing_player,
      int expanded,
      const std::optional<std::chrono::time_point<std::chrono::system_clock>>& deadline,
      PVInfo& pv_info,
      int null_moves = 0);

  std::optional<std::tuple<int, std::optional<Move>>> MTDF(
      Board& board,
      const std::optional<std::chrono::time_point<std::chrono::system_clock>>& deadline,
      int depth,
      int f);

//  std::optional<int> QuiescenceSearch(
//      Board& board,
//      int depth_left,
//      int alpha,
//      int beta,
//      bool maximizing_player,
//      int moves_since_last_active,
//      const std::optional<
//          std::chrono::time_point<std::chrono::system_clock>>& deadline);

  int GetNumEvaluations() { return num_evaluations_; }
  int GetNumCacheHits() { return num_cache_hits_; }
  int GetNumNullMovesTried() { return num_null_moves_tried_; }
  int GetNumNullMovesPruned() { return num_null_moves_pruned_; }
  int GetNumFutilityMovesPruned() { return num_futility_moves_pruned_; }
  int GetNumLmrSearches() { return num_lmr_searches_; }
  int GetNumLmrResearches() { return num_lmr_researches_; }

  ~AlphaBetaPlayer() {
    if (hash_table_ != nullptr) {
      delete[] hash_table_;
    }
  }

  void EnableDebug(bool enable) { enable_debug_ = enable; }
  void ResetMobilityScores(Board& board);
  int Reduction(int depth_left, int move_number) const;

 private:

  int StaticExchangeEvaluation(
      const Board& board,
      const Move& move) const;
  int StaticExchangeEvaluation(
      int square_piece_eval,
      const std::vector<int>& sorted_piece_values,
      size_t pos,
      const std::vector<int>& other_team_sorted_piece_values,
      size_t other_team_pos) const;
  std::vector<Move> PrunedMoveOrder(
      Board& board, bool maximizing_player, bool prune);
  void ResetHistoryHeuristic();
  void ResetHistoryCounters();

  int num_evaluations_ = 0; // debugging
  int num_cache_hits_ = 0;
  int num_null_moves_tried_ = 0;
  int num_null_moves_pruned_ = 0;
  int num_futility_moves_pruned_ = 0;
  int num_lmr_searches_ = 0;
  int num_lmr_researches_ = 0;
  PVInfo pv_info_;
  std::atomic<bool> canceled_ = false;
  int piece_evaluations_[6];
  int piece_move_order_scores_[6];
  PlayerOptions options_;
  int player_mobility_scores_[4];
  HashTableEntry* hash_table_ = nullptr;
  // TODO: use this if needed
  std::atomic<bool> enable_debug_ = false;

  // https://www.chessprogramming.org/History_Heuristic
  // (from_row, from_col, to_row, to_col)
  int history_heuristic_[14][14][14][14];
  // counters for each historical move (number of times encountered)
  // (player_color, from_row, from_col, to_row, to_col)
  int history_counter_[4][14][14][14][14];
  int reductions_[kMaxMoves];
};

}  // namespace chess

#endif  // _PLAYER_H_
