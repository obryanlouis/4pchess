#include "gmock/gmock.h"
#include <chrono>
#include <gtest/gtest.h>
#include <unordered_map>
#include <vector>

#include "utils.h"
#include "board.h"
#include "player.h"

namespace chess {

using ::testing::UnorderedElementsAre;

const Player kRedPlayer = Player(RED);
const Player kBluePlayer = Player(BLUE);
const Player kYellowPlayer = Player(YELLOW);
const Player kGreenPlayer = Player(GREEN);


TEST(PlayerTest, EvaluateCheckmate) {
  // Team is in checkmate already
  AlphaBetaPlayer player;

  std::unordered_map<BoardLocation, Piece> location_to_piece = {
    {BoardLocation(3, 0), Piece(kRedPlayer, KING)},
    {BoardLocation(5, 2), Piece(kBluePlayer, KING)},
    {BoardLocation(7, 7), Piece(kYellowPlayer, KING)},
    {BoardLocation(9, 9), Piece(kGreenPlayer, KING)},
    {BoardLocation(4, 1), Piece(kGreenPlayer, QUEEN)},
  };
  Board board(kRedPlayer, location_to_piece);

  const auto& res = player.MakeMove(board);
  ASSERT_TRUE(res.has_value());
  float valuation = std::get<0>(res.value());
  const auto& move_or = std::get<1>(res.value());
  EXPECT_FALSE(move_or.has_value());
  EXPECT_EQ(valuation, -kMateValue);
}

TEST(PlayerTest, EvaluateCheckmateNextMove) {
  // Team is in checkmate next move
  PlayerOptions options;
  AlphaBetaPlayer player(options);

  auto board = ParseBoardFromFEN("R-0,0,0,0-1,1,1,1-1,1,1,1-0,0,0,0-0-x,x,x,bK,7,x,x,x/x,x,x,7,rQ,x,x,x/x,x,x,2,yK,5,x,x,x/14/14/14/13,gK/14/14/14/14/x,x,x,8,x,x,x/x,x,x,8,x,x,x/x,x,x,4,rK,3,x,x,x");
  ASSERT_NE(board, nullptr);

  const auto& res = player.MakeMove(*board, std::nullopt, 2);
  ASSERT_TRUE(res.has_value());
  float valuation = std::get<0>(res.value());
  const auto& move_or = std::get<1>(res.value());
  EXPECT_TRUE(move_or.has_value());
  EXPECT_EQ(move_or.value(), Move(BoardLocation(1, 10), BoardLocation(1, 4)));
  EXPECT_EQ(valuation, kMateValue);
}

TEST(PlayerTest, EvaluateStalemate) {
  // Team is in stalemate
  AlphaBetaPlayer player;

  std::unordered_map<BoardLocation, Piece> location_to_piece = {
    {BoardLocation(3, 0), Piece(kRedPlayer, KING)},
    {BoardLocation(5, 2), Piece(kBluePlayer, KING)},
    {BoardLocation(7, 7), Piece(kYellowPlayer, KING)},
    {BoardLocation(9, 9), Piece(kGreenPlayer, KING)},
    {BoardLocation(4, 2), Piece(kGreenPlayer, QUEEN)},
  };
  Board board(kRedPlayer, location_to_piece);

  const auto& res = player.MakeMove(board, std::nullopt, 1);
  ASSERT_TRUE(res.has_value());
  float valuation = std::get<0>(res.value());
  const auto& move_or = std::get<1>(res.value());
  EXPECT_FALSE(move_or.has_value());
  EXPECT_EQ(valuation, 0);
}

TEST(PlayerTest, EvaluatePieceImbalance) {
  // Unequal pieces with a clear winning side
  AlphaBetaPlayer player;

  std::unordered_map<BoardLocation, Piece> location_to_piece = {
    {BoardLocation(3, 0), Piece(kBluePlayer, KING)},
    {BoardLocation(5, 2), Piece(kRedPlayer, KING)},
    {BoardLocation(7, 7), Piece(kGreenPlayer, KING)},
    {BoardLocation(9, 9), Piece(kYellowPlayer, KING)},
    {BoardLocation(4, 8), Piece(kYellowPlayer, QUEEN)},
  };
  Board board(kRedPlayer, location_to_piece);

  const auto& res = player.MakeMove(board, std::nullopt, 1);
  ASSERT_TRUE(res.has_value());
  float valuation = std::get<0>(res.value());
  const auto& move_or = std::get<1>(res.value());
  EXPECT_TRUE(move_or.has_value());
  EXPECT_GT(valuation, 0);
}

TEST(PlayerTest, EvaluateCheckmateForcedCheckmate1) {
  AlphaBetaPlayer player;

  auto board = Board::CreateStandardSetup();
  board->MakeMove(Move(BoardLocation(12, 7), BoardLocation(11, 7)));
  board->MakeMove(Move(BoardLocation(3, 1), BoardLocation(3, 2)));
  board->MakeMove(Move(BoardLocation(1, 8), BoardLocation(2, 8)));
  board->MakeMove(Move(BoardLocation(3, 12), BoardLocation(3, 11)));

  const auto& res = player.MakeMove(*board);
  ASSERT_TRUE(res.has_value());
  float valuation = std::get<0>(res.value());
  EXPECT_EQ(valuation, kMateValue);
  const auto& move_or = std::get<1>(res.value());
  ASSERT_TRUE(move_or.has_value());
  EXPECT_EQ(move_or.value(), Move(BoardLocation(13, 8), BoardLocation(11, 6)));
}

TEST(PlayerTest, CheckPVInfoProducesValidMoves) {
  AlphaBetaPlayer player;

  auto board = Board::CreateStandardSetup();
  board->MakeMove(Move(BoardLocation(12, 7), BoardLocation(11, 7)));
  board->MakeMove(Move(BoardLocation(7, 1), BoardLocation(7, 2)));
  board->MakeMove(Move(BoardLocation(1, 6), BoardLocation(2, 6)));
  board->MakeMove(Move(BoardLocation(6, 12), BoardLocation(6, 11)));

  constexpr int kDepth = 4;
  const auto& res = player.MakeMove(*board, std::nullopt, kDepth);
  ASSERT_TRUE(res.has_value());
  const auto& move_or = std::get<1>(res.value());
  ASSERT_TRUE(move_or.has_value());
  const PVInfo* pvinfo = &player.GetPVInfo();
  int num_pvmoves = 0;
  while (pvinfo != nullptr) {
    const auto& pvmove_or = pvinfo->GetBestMove();
    if (pvmove_or.has_value()) {
      num_pvmoves++;
      const auto& move = pvmove_or.value();
      Move moves[300];
      size_t num_moves = board->GetPseudoLegalMoves2(moves, 300);
      bool found = false;
      for (size_t i = 0; i < num_moves; i++) {
        if (moves[i] == move) {
          found = true;
          break;
        }
      }
      ASSERT_TRUE(found);
      board->MakeMove(move);
    }
    pvinfo = pvinfo->GetChild().get();
  }
  EXPECT_GE(num_pvmoves, kDepth);
}

//TEST(PlayerTest, StaticExchangeEvaluation) {
//  PlayerOptions options;
//  AlphaBetaPlayer player(options);
//
//  auto board = Board::CreateStandardSetup();
//
//  board->MakeMove(Move(BoardLocation(12, 7), BoardLocation(11, 7)));
//  board->MakeMove(Move(BoardLocation(7, 1), BoardLocation(7, 2)));
//  board->MakeMove(Move(BoardLocation(1, 6), BoardLocation(2, 6)));
//  board->MakeMove(Move(BoardLocation(6, 12), BoardLocation(6, 11)));
//
//  // Qg1 x m7
//  BoardLocation from(13, 6);
//  BoardLocation to(7, 12);
//  Move move(from, to, board->GetPiece(to));
//
//  int see = player.StaticExchangeEvaluationCapture(*board, move);
//
//  EXPECT_LT(see, 0);
//}

}  // namespace chess


