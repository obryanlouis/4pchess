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

constexpr size_t kTranspositionTableSize = 100'000'000;

struct PlayerOptions {
  bool enable_principal_variation = true;
  bool enable_static_exchange = false;
  bool enable_mobility_evaluation = true;
  bool enable_move_order = true;
  bool enable_move_order_checks = true;
  bool enable_late_move_reduction = true;
  bool enable_transposition_table = false;
  bool enable_quiescence = false;
  // https://www.chessprogramming.org/Aspiration_Windows
  bool enable_aspiration_window = false;
  bool enable_history_heuristic = true;

  // generic test change
  bool test = false;

  size_t transposition_table_size = kTranspositionTableSize;
  std::optional<int> max_search_depth;
};

enum ScoreBound {
  EXACT = 0, LOWER_BOUND = 1, UPPER_BOUND = 2,
};

struct HashTableEntry {
  int64_t key;
  int depth;
  std::optional<Move> best_move;
  float score;
  ScoreBound bound;
};

class AlphaBetaPlayer {
 public:
  AlphaBetaPlayer(std::optional<PlayerOptions> options = std::nullopt);
  std::optional<std::tuple<float, std::optional<Move>, int>> MakeMove(
      Board& board,
      std::optional<std::chrono::milliseconds> time_limit = std::nullopt,
      int max_depth = 20);
  float Evaluate(Board& board);
  std::vector<Move> MoveOrder(Board& board, bool maximizing_player);
  void CancelEvaluation() { canceled_ = true; }
  // NOTE: Should wait until evaluation is done before resetting this to true.
  void SetCanceled(bool canceled) { canceled_ = canceled; }
  bool IsCanceled() { return canceled_.load(); }
  const PVInfo& GetPVInfo() const { return pv_info_; }

  std::optional<std::pair<float, std::optional<Move>>> NegaMax(
      Board& board,
      int depth,
      int depth_left,
      float alpha,
      float beta,
      bool maximizing_player,
      int expanded,
      const std::optional<std::chrono::time_point<std::chrono::system_clock>>& deadline,
      PVInfo& pv_info);

  std::optional<float> QuiescenceSearch(
      Board& board,
      int depth_left,
      float alpha,
      float beta,
      bool maximizing_player,
      const std::optional<
          std::chrono::time_point<std::chrono::system_clock>>& deadline);

  int GetNumEvaluations() { return num_evaluations_; }
  int GetNumCacheHits() { return num_cache_hits_; }

  ~AlphaBetaPlayer() {
    if (hash_table_ != nullptr) {
      delete[] hash_table_;
    }
  }

  void EnableDebug(bool enable) { enable_debug_ = enable; }

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

  int num_evaluations_ = 0; // debugging
  int num_cache_hits_ = 0;
  PVInfo pv_info_;
  std::atomic<bool> canceled_ = false;
  int piece_evaluations_[6];
  float piece_move_order_scores_[6];
  PlayerOptions options_;
  float player_mobility_scores_[4];
  HashTableEntry* hash_table_ = nullptr;
  // TODO: use this if needed
  std::atomic<bool> enable_debug_ = false;

  // https://www.chessprogramming.org/History_Heuristic
  // (from_row, from_col, to_row, to_col)
  float history_heuristic_[14][14][14][14];
  //float history_heuristic_[14][14];
};

}  // namespace chess

#endif  // _PLAYER_H_
