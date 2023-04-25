#include <chrono>
#include <unordered_map>
#include <vector>
#include <gtest/gtest.h>
#include "gmock/gmock.h"

#include "board.h"
#include "player.h"

namespace chess {
namespace {

TEST(Speed, BoardTest) {
  auto start = std::chrono::system_clock::now();
  auto board = Board::CreateStandardSetup();
  PlayerOptions options;
  options.enable_futility_pruning = true;
  AlphaBetaPlayer player(options);
  player.EnableDebug(true);

  std::chrono::milliseconds time_limit(5000);
  auto res = player.MakeMove(*board, time_limit);

  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now() - start);
  std::cout << "Duration (ms): " << duration.count() << std::endl;
  int nps = (int) ((((float)player.GetNumEvaluations()) / duration.count())*1000.0);
  std::cout << "Nodes/sec: " << nps << std::endl;
  std::cout << "Nodes: " << player.GetNumEvaluations() << std::endl;
  if (options.enable_check_extensions) {
    std::cout << "#Check extensions: " << player.GetNumCheckExtensions() << std::endl;
  }
  if (options.enable_fail_high_reductions) {
    std::cout << "#Fail-high reductions: " << player.GetNumFailHighReductions() << std::endl;
  }
  if (options.enable_late_move_pruning) {
    std::cout << "#LM pruned: " << player.GetNumLateMovesPruned() << std::endl;
  }
  if (options.enable_singular_extensions) {
    std::cout << "#Singular searches: " << player.GetNumSingularExtensionSearches()
      << std::endl;
    std::cout << "#Singular extensions: " << player.GetNumSingularExtensions()
      << std::endl;
  }
  if (options.enable_transposition_table) {
    std::cout << "#Cache hits: " << player.GetNumCacheHits() << std::endl;
    float cache_hit_rate = (float)player.GetNumCacheHits() /
      (float)player.GetNumEvaluations();
    std::cout << "Cache hit rate: " << cache_hit_rate << std::endl;
  }
  if (options.enable_late_move_reduction) {
    std::cout << "#LMR searches: " << player.GetNumLmrSearches()
      << std::endl;
    std::cout << "#LMR re-searches: " << player.GetNumLmrResearches()
      << std::endl;
  }
  if (options.enable_null_move_pruning) {
    std::cout << "#Null moves tried: " << player.GetNumNullMovesTried()
      << std::endl;
    std::cout << "#Null moves pruned: " << player.GetNumNullMovesPruned()
      << std::endl;
  }
  if (options.enable_futility_pruning) {
    std::cout << "#Futility moves pruned: " << player.GetNumFutilityMovesPruned()
      << std::endl;
  }

  if (res.has_value() && std::get<1>(res.value()).has_value()) {
    const auto& val = res.value();
    std::cout << "Searched depth: " << std::get<2>(val) << std::endl;
    std::cout << "move: " << std::get<1>(val).value() << std::endl;
    std::cout << "evaluation: " << std::get<0>(val) << std::endl;
  } else {
    std::cout << "move missing" << std::endl;
  }

}

}  // namespace
}  // namespace chess

