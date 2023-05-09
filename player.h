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

 private:
  std::optional<Move> best_move_ = std::nullopt;
  std::shared_ptr<PVInfo> child_ = nullptr;
};

constexpr size_t kTranspositionTableSize = 10'000'000;
constexpr int kMaxPly = 300;
constexpr int kKillersPerPly = 3;

struct PlayerOptions {
  bool enable_move_order = true;
  bool enable_move_order_checks = true;
  bool enable_history_heuristic = true;
  bool enable_mobility_evaluation = true;
  bool pvs = true;
  bool enable_null_move_pruning = true;
  bool enable_late_move_reduction = true;
  bool enable_killers = true;
  bool enable_king_safety = true;
  bool enable_late_move_pruning = true;
  bool enable_transposition_table = true;
  bool enable_check_extensions = true;
  bool enable_piece_development = true;
  bool enable_lazy_eval = true;
  bool enable_piece_imbalance = true;
  bool enable_futility_pruning = true;

  // generic test change
  bool test = true;

  bool enable_history_leaf_pruning = false;

  size_t transposition_table_size = kTranspositionTableSize;
  std::optional<int> max_search_depth;
};

struct Stack {
  Move killers[2];
  std::optional<Move> excluded_move;
  bool tt_pv = false;
};

enum NodeType {
  NonPV,
  PV,
  Root,
};

class AlphaBetaPlayer {
 public:
  AlphaBetaPlayer(std::optional<PlayerOptions> options = std::nullopt);
  std::optional<std::tuple<int, std::optional<Move>, int>> MakeMove(
      Board& board,
      std::optional<std::chrono::milliseconds> time_limit = std::nullopt,
      int max_depth = 20);
  int Evaluate(Board& board, bool maximizing_player, int alpha = -kMateValue, int beta = kMateValue);
  void CancelEvaluation() { canceled_ = true; }
  // NOTE: Should wait until evaluation is done before resetting this to true.
  void SetCanceled(bool canceled) { canceled_ = canceled; }
  bool IsCanceled() { return canceled_.load(); }
  const PVInfo& GetPVInfo() const { return pv_info_; }

  std::optional<std::tuple<int, std::optional<Move>>> Search(
      Stack* ss,
      NodeType node_type,
      Board& board,
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

  std::optional<int> QuiescenceSearch(
      Board& board,
      int depth,
      int alpha,
      int beta,
      bool maximizing_player,
      const std::optional<
          std::chrono::time_point<std::chrono::system_clock>>& deadline,
      int msla = 0);

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

//  ~AlphaBetaPlayer() {
//    if (hash_table_ != nullptr) {
//      delete[] hash_table_;
//    }
//  }

  void EnableDebug(bool enable) { enable_debug_ = enable; }
  void ResetMobilityScores(Board& board);
  int Reduction(int depth, int move_number) const;

  // Returns SEE
  int StaticExchangeEvaluation(
      const Board& board, const BoardLocation& loc) const;

  // Returns SEE for a capture. Does not apply to en-passant capture.
  int StaticExchangeEvaluationCapture(
      Board& board, const Move& move) const;

  int ApproxSEECapture(
      Board& board, const Move& move) const;

  // Helper for SEE calculation
  int StaticExchangeEvaluation(
      int square_piece_eval,
      const std::vector<int>& sorted_piece_values,
      size_t pos,
      const std::vector<int>& other_team_sorted_piece_values,
      size_t other_team_pos) const;

 private:

  void ResetHistoryHeuristic();
  void ResetHistoryCounters();
  void UpdateQuietStats(Stack* ss, const Move& move);
  int MobilityEvaluation(Board& board, Player turn);

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

  PVInfo pv_info_;
  std::atomic<bool> canceled_ = false;
  int piece_evaluations_[6];
  int piece_move_order_scores_[6];
  PlayerOptions options_;
  int player_mobility_scores_[4] = {0, 0, 0, 0};
  int location_evaluations_[14][14];

  //HashTableEntry* hash_table_ = nullptr;
  std::unique_ptr<TranspositionTable> transposition_table_;

  std::atomic<bool> enable_debug_ = false;

  // https://www.chessprogramming.org/History_Heuristic
  // (from_row, from_col, to_row, to_col)
  int history_heuristic_[14][14][14][14];
  // counters for each historical move (number of times encountered)
  // (player_color, from_row, from_col, to_row, to_col)
  int history_counter_[4][14][14][14][14];
  int reductions_[kMaxPly];
  int root_depth_;
  int late_move_pruning_[20];

  // For evaluation
  int king_attack_weight_[30];
  int threat_by_minor_[kNumPieceTypes];
  int threat_by_rook_[kNumPieceTypes];
  int threat_by_king_ = 24;
  int threat_hanging_ = 72;
  int weak_queen_protection_ = 12;
  int king_attacker_values_[6];
};

}  // namespace chess

#endif  // _PLAYER_H_
