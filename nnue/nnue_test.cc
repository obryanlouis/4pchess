#include <chrono>
#include <unordered_map>
#include <vector>
#include <gtest/gtest.h>
#include "gmock/gmock.h"

#include "nnue.h"
#include "../board.h"

namespace chess {
namespace {

TEST(NNUE, SpeedTest) {
  NNUE nnue("/home/louis/4pchess/models");
  auto start = std::chrono::system_clock::now();

  constexpr int kNumEvals = 1000000;
  for (int i = 0; i < kNumEvals; i++) {
    nnue.Evaluate(RED);
  }

  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now() - start);
  std::cout << "Duration (ms): " << duration.count() << std::endl;
  std::cout << "Evals: " << kNumEvals << std::endl;
  float evals_per_sec = 1000 * kNumEvals / duration.count();
  std::cout << "Evals/sec: " << evals_per_sec << std::endl;
}

TEST(NNUE, CorrectnessTest) {
  NNUE nnue("/home/louis/4pchess/models");

  constexpr int kNumEvals = 1000000;

  auto board = Board::CreateStandardSetup();
  board->SetNNUE(&nnue);

  std::vector<PlacedPiece> pp;
  for (const auto& placed_pieces : board->GetPieceList()) {
    pp.insert(pp.end(), placed_pieces.begin(), placed_pieces.end());
  }
  nnue.InitializeWeights(pp);

  board->MakeMove(Move(BoardLocation(13, 7), BoardLocation(12, 7)));

  int pred_score = nnue.Evaluate(YELLOW);

  std::cout << "pred score: " << pred_score << std::endl;
}

TEST(NNUE, CopyWeightsTest) {
  std::string weights_dir = "/home/louis/4pchess/models";
  auto copy_from = std::make_shared<NNUE>(weights_dir);
  NNUE nnue(weights_dir, copy_from);

  constexpr int kNumEvals = 1000000;

  auto board = Board::CreateStandardSetup();
  board->SetNNUE(&nnue);

  std::vector<PlacedPiece> pp;
  for (const auto& placed_pieces : board->GetPieceList()) {
    pp.insert(pp.end(), placed_pieces.begin(), placed_pieces.end());
  }
  nnue.InitializeWeights(pp);

  board->MakeMove(Move(BoardLocation(13, 7), BoardLocation(12, 7)));

  int pred_score = nnue.Evaluate(YELLOW);

  std::cout << "pred score: " << pred_score << std::endl;
}

TEST(NNUE, NaiveFloatTest) {
  auto start = std::chrono::system_clock::now();

  constexpr size_t kNumEvals = 10000000000;
  float r = 0;
  for (size_t i = 0; i < kNumEvals; i++) {
    r += i;
  }

  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now() - start);
  std::cout << "Duration (ms): " << duration.count() << std::endl;
  std::cout << "Float ops: " << kNumEvals << std::endl;
  int64_t evals_per_sec = (int64_t) (1000 * kNumEvals / duration.count());
  std::cout << "Float ops/sec: " << evals_per_sec << std::endl;
  std::cout << "Res: " << r << std::endl;
}

TEST(NNUE, NaiveInt32Test) {
  auto start = std::chrono::system_clock::now();

  constexpr size_t kNumEvals = 10000000000;
  int32_t r = 0;
  for (size_t i = 0; i < kNumEvals; i++) {
    r += i;
  }

  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now() - start);
  std::cout << "Duration (ms): " << duration.count() << std::endl;
  std::cout << "Int32 ops: " << kNumEvals << std::endl;
  int64_t evals_per_sec = (int64_t) (1000 * kNumEvals / duration.count());
  std::cout << "Int32 ops/sec: " << evals_per_sec << std::endl;
  std::cout << "Res: " << r << std::endl;
}

}  // namespace
}  // namespace chess

