#include <gtest/gtest.h>
#include <iostream>

#include "player.h"
#include "board.h"
#include "utils.h"

namespace chess {

namespace {

std::unique_ptr<AlphaBetaPlayer> CreatePlayer() {
  PlayerOptions options;
  // the test is more stable but less realistic with 1 thread.
//  options.enable_multithreading = true;
//  options.num_threads = 1;
  return std::make_unique<AlphaBetaPlayer>(options);
}

}  // namespace

TEST(PlayerTest, EvaluateCheckmate1) {
  auto player = CreatePlayer();

  // https://www.chess.com/variants/4-player-chess/game/47918736/26/1
  // move 7y
  std::string fen = "Y-0,0,0,0-1,1,1,1-1,1,1,1-0,0,0,0-2-x,x,x,yR,1,yB,yK,1,yB,yN,yR,x,x,x/x,x,x,yP,yP,yP,1,yP,gQ,yP,yP,x,x,x/x,x,x,2,yN,yP,4,x,x,x/bR,bP,1,bN,8,gP,gR/1,bP,10,gP,gN/bB,bP,10,gP,gB/1,bP,9,gP,1,gK/bK,1,bP,9,gP,1/1,bP,bB,9,gP,gB/12,gP,1/bR,bP,2,yQ,5,gN,1,gP,gR/x,x,x,2,rQ,1,rP,3,x,x,x/x,x,x,rP,rP,rP,rP,1,rP,rP,rP,x,x,x/x,x,x,rR,1,rB,1,rK,rB,rN,rR,x,x,x";

  auto board = ParseBoardFromFEN(fen);

  auto res = player->MakeMove(*board, std::nullopt, 5);
  const auto move_or = std::get<1>(*res);

  // Make sure that we avoid mate in 1.
  board->MakeMove(*move_or);
  res = player->MakeMove(*board, std::nullopt, 5);
  float valuation = std::get<0>(*res);
  EXPECT_GT(valuation, -kMateValue);
}

TEST(PlayerTest, EvaluateCheckmate2) {
  auto player = CreatePlayer();

  // https://www.chess.com/variants/custom/game/46170950/36/1
  // move 10r
  std::string fen = "R-0,0,0,0-1,0,1,1-1,0,1,1-0,0,0,0-1-x,x,x,yR,1,yB,yK,2,yN,yR,x,x,x/x,x,x,yP,yP,yP,yP,2,yP,yP,x,x,x/x,x,x,2,yN,2,yP,2,x,x,x/bR,bP,5,yP,4,gP,gR/bN,bP,10,gP,1/bB,2,yQ,7,gN,gP,gB/1,bP,rQ,1,rB,6,gP,1,gK/bK,1,bP,2,bQ,6,gP,1/1,bP,1,bN,4,gQ,3,gP,gB/bR,9,gP,3/1,bP,2,rP,4,gP,gN,2,gR/x,x,x,2,rN,1,rP,rN,2,x,x,x/x,x,x,rP,1,rP,rP,1,rP,rP,rP,x,x,x/x,x,x,rR,3,rK,rB,1,rR,x,x,x";

  auto board = ParseBoardFromFEN(fen);

  // best: 8
  auto res = player->MakeMove(*board, std::nullopt, 20);
  float valuation = std::get<0>(*res);

  EXPECT_EQ(valuation, kMateValue);
}

TEST(PlayerTest, EvaluateCheckmate3) {
  auto player = CreatePlayer();

  std::string fen = "R-0,0,0,0-1,1,1,1-1,1,1,1-0,0,0,0-0-x,x,x,yR,yN,1,yK,1,yB,yN,yR,x,x,x/x,x,x,yP,yP,yP,1,yP,yP,yP,yP,x,x,x/x,x,x,3,yP,4,x,x,x/bR,bP,2,yQ,3,yB,3,gP,gR/bN,bP,10,gP,gN/bB,2,bP,8,gP,gB/bQ,bP,9,gP,1,gK/bK,bP,bP,2,gQ,6,gP,1/bB,11,gP,gB/bN,1,bP,9,gP,gN/bR,bP,10,gP,gR/x,x,x,2,rP,1,rP,3,x,x,x/x,x,x,rP,rP,1,rP,1,rP,rP,rP,x,x,x/x,x,x,rR,rN,rB,rQ,rK,1,rN,rR,x,x,x";

  auto board = ParseBoardFromFEN(fen);

  auto res = player->MakeMove(*board, std::nullopt, 6);
  float valuation = std::get<0>(*res);

  EXPECT_EQ(valuation, kMateValue);
}

TEST(PlayerTest, EvaluateCheckmate4) {
  auto player = CreatePlayer();

  std::string fen = "Y-0,0,0,0-1,0,0,1-1,1,1,1-15,18,9,5-0-x,x,x,2,yB,yK,3,yR,x,x,x/x,x,x,1,yP,yP,yN,yQ,yP,2,x,x,x/x,x,x,4,yP,yN,yP,1,x,x,x/bR,bP,1,bP,2,yP,2,yP,3,gR/12,gP,1/bB,bP,9,gN,gP,gB/1,bP,10,gP,gK/bK,1,bP,7,gP,3/1,bP,9,gP,1,gB/7,gQ,4,gP,gN/rB,2,bP,rP,5,rP,1,gP,gR/x,x,x,2,bN,1,rP,rN,2,x,x,x/x,x,x,rP,1,rP,rP,rB,rP,rP,1,x,x,x/x,x,x,rR,3,rK,2,rR,x,x,x";

  auto board = ParseBoardFromFEN(fen);

  // best: 4
  auto res = player->MakeMove(*board, std::nullopt, 16);
  float valuation = std::get<0>(*res);

  EXPECT_EQ(valuation, kMateValue);
}

TEST(PlayerTest, EvaluateNonCheckmate5) {
  auto player = CreatePlayer();

  std::string fen = "G-0,0,0,0-0,1,1,0-0,1,1,0-0,0,0,0-1-x,x,x,yR,1,yB,yK,3,yR,x,x,x/x,x,x,yP,yP,yP,1,yP,yP,yB,yP,x,x,x/x,x,x,3,yP,rQ,1,yP,1,x,x,x/bR,bP,1,yN,8,gP,gR/3,bP,5,yN,2,gP,gN/bB,bP,11,gB/1,bP,9,gP,2/bK,1,bP,9,gP,gK/1,bP,9,gP,1,gB/1,bP,10,gP,gN/bR,bP,10,gP,gR/x,x,x,2,bB,rP,1,rP,2,x,x,x/x,x,x,1,rP,rP,1,rK,1,rB,gQ,x,x,x/x,x,x,rR,1,bN,5,x,x,x";

  auto board = ParseBoardFromFEN(fen);

  auto res = player->MakeMove(*board, std::nullopt, 1);
  float valuation = std::get<0>(*res);

  EXPECT_LT(valuation, 0);
  EXPECT_GT(valuation, -kMateValue);
}

TEST(PlayerTest, EvaluateNonCheckmate6) {
  auto player = CreatePlayer();

  std::string fen = "G-0,0,0,0-1,1,1,1-1,1,1,1-0,0,0,0-0-x,x,x,yR,yN,1,yK,yQ,yB,yN,yR,x,x,x/x,x,x,yP,yP,yP,1,yP,yP,yP,yP,x,x,x/x,x,x,3,yP,4,x,x,x/bR,bP,10,gP,gR/bN,bP,10,gP,gN/bB,bP,10,gP,gB/1,bP,9,yB,1,gK/bK,1,bP,9,gP,gQ/bB,bP,10,gP,gB/bN,bP,1,bQ,8,gP,gN/bR,bP,7,rQ,2,gP,gR/x,x,x,4,rP,3,x,x,x/x,x,x,rP,rP,rP,rP,1,rP,rP,rP,x,x,x/x,x,x,rR,rN,rB,1,rK,rB,rN,rR,x,x,x";

  auto board = ParseBoardFromFEN(fen);

  auto res = player->MakeMove(*board, std::nullopt, 12);
  float valuation = std::get<0>(*res);
  auto move = std::get<1>(*res);

  EXPECT_GT(valuation, -300);
  EXPECT_LT(valuation, 300);

  auto bad_move = ParseMove(*board, "m9-l8");
  ASSERT_TRUE(bad_move.has_value());
  EXPECT_NE(move, bad_move);
}


}  // namespace chess
