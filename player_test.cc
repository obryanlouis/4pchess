#include "gmock/gmock.h"
#include <chrono>
#include <gtest/gtest.h>
#include <unordered_map>
#include <vector>

#include "board.h"
#include "player.h"

namespace chess {

using ::testing::UnorderedElementsAre;

const Player kRedPlayer = Player(RED);
const Player kBluePlayer = Player(BLUE);
const Player kYellowPlayer = Player(YELLOW);
const Player kGreenPlayer = Player(GREEN);

constexpr int kMateValue = 1000000'00;  // mate value (centipawns)


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

  std::unordered_map<BoardLocation, Piece> location_to_piece = {
    {BoardLocation(13, 8), Piece(kRedPlayer, KING)},
    {BoardLocation(0, 3), Piece(kBluePlayer, KING)},
    {BoardLocation(0, 5), Piece(kYellowPlayer, KING)},
    {BoardLocation(6, 13), Piece(kGreenPlayer, KING)},
    {BoardLocation(1, 10), Piece(kRedPlayer, QUEEN)},
  };
  Board board(kRedPlayer, location_to_piece);

  const auto& res = player.MakeMove(board, std::nullopt, 2);
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
      const auto& moves = board->GetPseudoLegalMoves();
      auto it = std::find(moves.begin(), moves.end(), move);
      ASSERT_NE(it, moves.end());
      board->MakeMove(move);
    }
    pvinfo = pvinfo->GetChild().get();
  }
  EXPECT_EQ(num_pvmoves, kDepth);
}

TEST(PlayerTest, StaticExchangeEvaluation) {
  PlayerOptions options;
  AlphaBetaPlayer player(options);

  auto board = Board::CreateStandardSetup();

  board->MakeMove(Move(BoardLocation(12, 7), BoardLocation(11, 7)));
  board->MakeMove(Move(BoardLocation(7, 1), BoardLocation(7, 2)));
  board->MakeMove(Move(BoardLocation(1, 6), BoardLocation(2, 6)));
  board->MakeMove(Move(BoardLocation(6, 12), BoardLocation(6, 11)));

  // Qg1 x m7
  BoardLocation from(13, 6);
  BoardLocation to(7, 12);
  Move move(from, to, board->GetPiece(to));

  int see = player.StaticExchangeEvaluationCapture(*board, move);

  EXPECT_LT(see, 0);
}

TEST(PlayerTest, Quiescence) {
  PlayerOptions options;
  options.enable_quiescence = true;
  AlphaBetaPlayer player(options);

  auto board = Board::CreateStandardSetup();

  const auto& res = player.MakeMove(*board, std::nullopt, 1);
  ASSERT_TRUE(res.has_value());
  auto eval = std::get<0>(res.value());
  auto move = std::get<1>(res.value());
  ASSERT_TRUE(move.has_value());
//  std::cout << "eval: " << eval << std::endl;
//  std::cout << "best move: " << move.value() << std::endl;

  EXPECT_GT(eval, -kMateValue);
  EXPECT_LT(eval, kMateValue);
}

}  // namespace chess


