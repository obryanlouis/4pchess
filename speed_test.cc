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
  options.enable_quiescence = true;
  AlphaBetaPlayer player(options);

  std::chrono::milliseconds time_limit(10000);
  auto res = player.MakeMove(*board, time_limit);

  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now() - start);
  std::cout << "Duration (ms): " << duration.count() << std::endl;
  int nps = (int) ((((float)player.GetNumEvaluations()) / duration.count())*1000.0);
  std::cout << "Nodes/sec: " << nps << std::endl;
  if (options.enable_transposition_table) {
    float cache_hit_rate = (float)player.GetNumCacheHits() /
      (float)player.GetNumEvaluations();
    std::cout << "Cache hit rate: " << cache_hit_rate << std::endl;
  }

  if (res.has_value()) {
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

