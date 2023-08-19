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
#include "transposition_table.h"

namespace chess {

constexpr int kMateValue = 1000000'00;  // mate value (centipawns)

class PVInfo {
 public:
  PVInfo() = default;

  const std::optional<Move>& GetBestMove() const { return best_move_; }
  std::shared_ptr<PVInfo> GetChild() const { return child_; }
  void SetBestMove(Move move) { best_move_ = std::move(move); }
  void SetChild(std::shared_ptr<PVInfo> child) { child_ = std::move(child); }
  int GetDepth() const;

  std::shared_ptr<PVInfo> Copy() const;

 private:
  std::optional<Move> best_move_ = std::nullopt;
  std::shared_ptr<PVInfo> child_ = nullptr;
};

constexpr size_t kTranspositionTableSize = 2'000'000;
constexpr int kMaxPly = 300;
constexpr int kKillersPerPly = 3;

struct PlayerOptions {
  bool enable_move_order = true;
  bool enable_move_order_checks = true;
  bool enable_history_heuristic = true;
  bool enable_mobility_evaluation = true;
  bool pvs = true;
  bool enable_killers = true;
  bool enable_king_safety = true;
  bool enable_pawn_shield = true;
  bool enable_attacking_king_zone = true;
  bool enable_transposition_table = true;
  bool enable_check_extensions = true;
  bool enable_piece_imbalance = true;
  bool enable_lazy_eval = true;
  bool enable_piece_activation = true;

  bool enable_late_move_reduction = true;
  bool enable_late_move_pruning =   true;
  bool enable_null_move_pruning =   true;

  bool enable_multithreading = true;
  int num_threads = 12;

  // generic test change
  bool test = false;

  bool enable_piece_square_table = false;

  size_t transposition_table_size = kTranspositionTableSize;
  std::optional<int> max_search_depth;
};

struct Stack {
  Move killers[2];
  bool tt_pv = false;
  int move_count = 0;
};

enum NodeType {
  NonPV,
  PV,
  Root,
};

constexpr size_t kBufferPartitionSize = 300; // number of elements per buffer partition
constexpr size_t kBufferNumPartitions = 200; // number of recursive calls

// Manages state of worker threads during search
class ThreadState {
 public:
  ThreadState(
      PlayerOptions options, const Board& board, const PVInfo& pv_info);
  Board& GetBoard() { return board_; }
  Move* GetNextMoveBufferPartition();
  void ReleaseMoveBufferPartition();
  int* NActivated() { return n_activated_; }
  int* TotalMoves() { return total_moves_; }
  PVInfo& GetPVInfo() { return pv_info_; }
  void ResetHistoryHeuristic();

  ~ThreadState();

  // https://www.chessprogramming.org/History_Heuristic
  // (from_row, from_col, to_row, to_col)
  int history_heuristic[14][14][14][14];

 private:
  PlayerOptions options_;
  Board board_;
  PVInfo pv_info_;

  // Buffer used to store moves per node.
  // Each node generates up to `partition_size` moves, and there
  Move* move_buffer_ = nullptr;
  // Id within move_buffer_
  size_t buffer_id_ = 0;

  int n_activated_[4] = {0, 0, 0, 0};
  int total_moves_[4] = {0, 0, 0, 0};

};

class AlphaBetaPlayer {
 public:
  AlphaBetaPlayer(
      std::optional<PlayerOptions> options = std::nullopt);
  std::optional<std::tuple<int, std::optional<Move>, int>> MakeMove(
      Board& board,
      std::optional<std::chrono::milliseconds> time_limit = std::nullopt,
      int max_depth = 20);
  int Evaluate(ThreadState& thread_state, bool maximizing_player,
      int alpha = -kMateValue, int beta = kMateValue);
  void CancelEvaluation() { canceled_ = true; }
  // NOTE: Should wait until evaluation is done before resetting this to true.
  void SetCanceled(bool canceled) { canceled_ = canceled; }
  //bool IsCanceled() { return canceled_.load(); }
  bool IsCanceled() { return canceled_; }
  const PVInfo& GetPVInfo() const { return pv_info_; }

  std::optional<std::tuple<int, std::optional<Move>>> Search(
      Stack* ss,
      NodeType node_type,
      ThreadState& thread_state,
      int ply,
      int depth,
      int alpha,
      int beta,
      bool maximizing_player,
      int expanded,
      const std::optional<std::chrono::time_point<std::chrono::system_clock>>& deadline,
      PVInfo& pv_info,
      int null_moves = 0,
      bool isCutNode = false);

  int64_t GetNumEvaluations() { return num_nodes_; }
  int64_t GetNumCacheHits() { return num_cache_hits_; }
  int64_t GetNumNullMovesTried() { return num_null_moves_tried_; }
  int64_t GetNumNullMovesPruned() { return num_null_moves_pruned_; }
  int64_t GetNumFutilityMovesPruned() { return num_futility_moves_pruned_; }
  int64_t GetNumLmrSearches() { return num_lmr_searches_; }
  int64_t GetNumLmrResearches() { return num_lmr_researches_; }
  int64_t GetNumSingularExtensionSearches() {
    return num_singular_extension_searches_;
  }
  int64_t GetNumSingularExtensions() {
    return num_singular_extensions_;
  }
  int64_t GetNumLateMovesPruned() { return num_lm_pruned_; }
  int64_t GetNumFailHighReductions() { return num_fail_high_reductions_; }
  int64_t GetNumCheckExtensions() { return num_check_extensions_; }
  int64_t GetNumLazyEval() { return num_lazy_eval_; }

  void EnableDebug(bool enable) { enable_debug_ = enable; }
  int Reduction(int depth, int move_number) const;

 private:

  std::optional<std::tuple<int, std::optional<Move>, int>>
    MakeMoveSingleThread(
      ThreadState& state,
      std::optional<std::chrono::time_point<std::chrono::system_clock>> deadline,
      int max_depth = 20);

  void ResetMobilityScores(ThreadState& thread_state);
  void UpdateQuietStats(Stack* ss, const Move& move);
  void UpdateMobilityEvaluation(ThreadState& thread_state, Player turn);
  bool HasShield(Board& board, PlayerColor color, const BoardLocation& king_loc);
  bool OnBackRank(const BoardLocation& king_loc);

  int64_t num_nodes_ = 0; // debugging
  int64_t num_quiescence_nodes_ = 0;
  int64_t num_cache_hits_ = 0;
  int64_t num_null_moves_tried_ = 0;
  int64_t num_null_moves_pruned_ = 0;
  int64_t num_futility_moves_pruned_ = 0;
  int64_t num_lmr_searches_ = 0;
  int64_t num_lmr_researches_ = 0;
  int64_t num_singular_extension_searches_ = 0;
  int64_t num_singular_extensions_ = 0;
  int64_t num_lm_pruned_ = 0;
  int64_t num_fail_high_reductions_ = 0;
  int64_t num_check_extensions_ = 0;
  int64_t num_lazy_eval_ = 0;

  bool canceled_ = false;
  int piece_evaluations_[6];
  int piece_move_order_scores_[6];
  PlayerOptions options_;
  int location_evaluations_[14][14];

  //HashTableEntry* hash_table_ = nullptr;
  std::unique_ptr<TranspositionTable> transposition_table_;
  PVInfo pv_info_;

  bool enable_debug_ = false;

  // https://www.chessprogramming.org/History_Heuristic
  // (from_row, from_col, to_row, to_col)
  //int history_heuristic_[14][14][14][14];
  int reductions_[kMaxPly];

  // For evaluation
  int king_attack_weight_[30];
  int king_attacker_values_[6];
  // color x piece type x row x col
  int piece_square_table_[4][6][14][14];
  // number of moves a piece needs to have to be considered active
  int piece_activation_threshold_[7];

  int searching_[kMaxPly][1000];
};

}  // namespace chess

#endif  // _PLAYER_H_
