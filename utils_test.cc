#include "gmock/gmock.h"
#include <chrono>
#include <gtest/gtest.h>
#include <unordered_map>
#include <vector>

#include "board.h"
#include "player.h"
#include "utils.h"

namespace chess {

using ::testing::UnorderedElementsAre;


TEST(UtilsTest, ParseFENTest) {
  auto board = ParseBoardFromFEN("R-0,0,0,0-1,1,1,1-1,1,1,0-0,0,0,0-0-x,x,x,yR,yN,yB,1,yK,yB,1,yR,x,x,x/x,x,x,1,yP,1,yP,1,yP,yP,1,x,x,x/x,x,x,2,yP,1,yP,yN,2,x,x,x/bR,bP,1,yP,6,yP,1,gP,1/1,bP,11,gR/bB,10,gP,gP,gB/1,bP,2,bN,7,gP,1/bK,1,bP,8,gP,1,gK/bB,bP,10,gP,gB/bN,bP,bQ,7,rP,1,gP,gN/bR,bP,4,rQ,6,gR/x,x,x,2,rP,2,gP,2,x,x,x/x,x,x,rP,rP,1,rP,rP,rP,1,rP,x,x,x/x,x,x,rR,rN,rB,1,rK,rB,1,rR,x,x,x");
  EXPECT_NE(board, nullptr);
}

TEST(UtilsTest, ParseFENWithEnpassantTest) {
  auto board = ParseBoardFromFEN("Y-0,0,0,0-1,1,1,1-1,1,1,1-0,0,0,0-0-{'enPassant':('','c4:d4','','')}-x,x,x,yR,yN,yB,yK,yQ,yB,yN,yR,x,x,x/x,x,x,yP,yP,yP,yP,yP,yP,yP,yP,x,x,x/x,x,x,8,x,x,x/bR,bP,10,gP,gR/bN,bP,10,gP,gN/bB,bP,10,gP,gB/bQ,bP,10,gP,gK/bK,bP,10,gP,gQ/bB,bP,10,gP,gB/bN,bP,10,gP,gN/bR,2,bP,8,gP,gR/x,x,x,4,rP,3,x,x,x/x,x,x,rP,rP,rP,rP,1,rP,rP,rP,x,x,x/x,x,x,rR,rN,rB,rQ,rK,rB,rN,rR,x,x,x");
  EXPECT_NE(board, nullptr);
  EXPECT_EQ(board->GetEnpassantInitialization().enp_moves[BLUE],
            Move(BoardLocation(10, 1), BoardLocation(10, 3)));
}

TEST(UtilsTest, ParseFENWithError) {
  auto board = ParseBoardFromFEN("B-0,0,0,0-1,1,1,1-1,1,1,1-0,0,0,0-0-x,x,x,yR,yN,yB,yK,1,yB,yN,yR,x,x,x/x,x,x,1,yP,yP,1,yP,yP,yP,yP,x,x,x/x,x,x,3,yP,4,x,x,x/bR,1,bP,yP,8,gP,gR/bN,bP,9,gP,1,gN/bB,2,bP,7,gP,1,gB/bQ,bP,8,gP,2,gK/bK,rQ,9,gP,gQ,1/rB,yQ,2,bP,7,gP,gB/bN,2,bP,8,gP,gN/bR,bP,2,rP,7,gP,gR/x,x,x,4,rP,3,x,x,x/x,x,x,rP,1,rP,rP,1,rP,rP,rP,x,x,x/x,x,x,rR,rN,2,rK,rB,rN,rR,x,x,x");
  AlphaBetaPlayer player;
  std::chrono::milliseconds time_limit(950);
  auto res = player.MakeMove(*board, time_limit);
  EXPECT_TRUE(res.has_value());
}

TEST(UtilsTest, ParseFENWithError2) {
  auto board = ParseBoardFromFEN("R-0,0,0,0-0,0,1,1-1,1,1,1-0,0,0,0-0-{'enPassant':('','','e12:e11','')}-x,x,x,yR,yN,1,yK,1,yB,yN,yR,x,x,x/x,x,x,yP,1,yP,1,yP,yP,yP,yP,x,x,x/x,x,x,3,yP,yB,3,x,x,x/bR,bP,2,yP,7,gP,gR/bN,2,bP,8,gP,1/2,bP,8,gN,gP,gB/1,bB,9,gP,1,gK/bK,1,bP,9,gP,gQ/bB,bP,9,gP,2/bN,1,bP,7,gP,2,gN/rQ,bP,10,gP,gR/x,x,x,4,rP,3,x,x,x/x,x,x,rP,rP,rP,rP,1,rP,1,rP,x,x,x/x,x,x,rR,rN,rB,1,rK,1,rN,gB,x,x,x");
  AlphaBetaPlayer player;
  std::chrono::milliseconds time_limit(950);
  auto res = player.MakeMove(*board, time_limit);
  EXPECT_TRUE(res.has_value());
}

TEST(UtilsTest, ParseEnpLocation) {
  auto enp_location = ParseEnpLocation("c4:d4");
  EXPECT_EQ(enp_location, BoardLocation(10, 3));
}

TEST(UtilsTest, ParseMoveTest) {
  std::shared_ptr<Board> board;
  std::optional<Move> move_or;

  board = Board::CreateStandardSetup();
  move_or = ParseMove(*board, "h2h3");
  EXPECT_EQ(move_or, Move(BoardLocation(12, 7), BoardLocation(11, 7)));

  board = Board::CreateStandardSetup();
  move_or = ParseMove(*board, "h2-h3");
  EXPECT_EQ(move_or, Move(BoardLocation(12, 7), BoardLocation(11, 7)));

  board = Board::CreateStandardSetup();
  board->SetPlayer(Player(YELLOW));
  move_or = ParseMove(*board, "g13g12");
  EXPECT_EQ(move_or, Move(BoardLocation(1, 6), BoardLocation(2, 6)));

  board = Board::CreateStandardSetup();
  board->SetPlayer(Player(BLUE));
  move_or = ParseMove(*board, "b7c7");
  EXPECT_EQ(move_or, Move(BoardLocation(7, 1), BoardLocation(7, 2)));

  board = Board::CreateStandardSetup();
  board->SetPlayer(Player(RED));
  board->MakeMove(Move(BoardLocation(12, 7), BoardLocation(11, 7)));
  board->SetPlayer(Player(RED));
  move_or = ParseMove(*board, "Qg1-j4");
  EXPECT_EQ(move_or, Move(BoardLocation(13, 6), BoardLocation(10, 9)));
  move_or = ParseMove(*board, "g1-j4");
  EXPECT_EQ(move_or, Move(BoardLocation(13, 6), BoardLocation(10, 9)));
  move_or = ParseMove(*board, "Qg1xm7");
  EXPECT_EQ(move_or, Move(BoardLocation(13, 6), BoardLocation(7, 12),
                          board->GetPiece(7, 12)));
  move_or = ParseMove(*board, "g1m7");
  EXPECT_EQ(move_or, Move(BoardLocation(13, 6), BoardLocation(7, 12),
                          board->GetPiece(7, 12)));

  board = Board::CreateStandardSetup();
  board->MakeMove(*ParseMove(*board, "h2-h3"));
  board->MakeMove(*ParseMove(*board, "b7-c7"));

  move_or = ParseMove(*board, "g13-g12");
  EXPECT_EQ(move_or, Move(BoardLocation(1, 6), BoardLocation(2, 6)));
}

// test parsing promotions
TEST(UtilsTest, ParsePromotionTest) {
  std::shared_ptr<Board> board;
  std::optional<Move> move_or;

  // quiet move
  board = ParseBoardFromFEN("R-0,0,0,0-1,1,1,1-1,1,1,1-0,0,0,0-0-{'enPassant':('','','','')}-x,x,x,yR,yN,yB,yK,yQ,yB,yN,yR,x,x,x/x,x,x,yP,yP,yP,yP,yP,yP,yP,yP,x,x,x/x,x,x,8,x,x,x/bR,bP,4,bN,1,gN,3,gP,gR/1,bP,5,rP,4,gP,1/bB,bP,10,gP,gB/bQ,bP,10,gP,gK/bK,bP,10,gP,gQ/bB,bP,10,gP,gB/bN,bP,10,gP,gN/bR,bP,10,gP,gR/x,x,x,8,x,x,x/x,x,x,rP,rP,rP,rP,1,rP,rP,rP,x,x,x/x,x,x,rR,rN,rB,rQ,rK,rB,rN,rR,x,x,x");
  ASSERT_NE(board, nullptr);
  EXPECT_EQ(board->GetTurn().GetColor(), RED);

  // all piece types
  move_or = ParseMove(*board, "h10h11q");
  EXPECT_EQ(move_or, Move(BoardLocation(4, 7), BoardLocation(3, 7),
                          Piece::kNoPiece, BoardLocation::kNoLocation,
                          Piece::kNoPiece, QUEEN));

  move_or = ParseMove(*board, "h10h11r");
  EXPECT_EQ(move_or, Move(BoardLocation(4, 7), BoardLocation(3, 7),
                          Piece::kNoPiece, BoardLocation::kNoLocation,
                          Piece::kNoPiece, ROOK));

  move_or = ParseMove(*board, "h10h11b");
  EXPECT_EQ(move_or, Move(BoardLocation(4, 7), BoardLocation(3, 7),
                          Piece::kNoPiece, BoardLocation::kNoLocation,
                          Piece::kNoPiece, BISHOP));

  move_or = ParseMove(*board, "h10h11n");
  EXPECT_EQ(move_or, Move(BoardLocation(4, 7), BoardLocation(3, 7),
                          Piece::kNoPiece, BoardLocation::kNoLocation,
                          Piece::kNoPiece, KNIGHT));

  // upper/lower cases both accepted
  move_or = ParseMove(*board, "h10h11Q");
  EXPECT_EQ(move_or, Move(BoardLocation(4, 7), BoardLocation(3, 7),
                          Piece::kNoPiece, BoardLocation::kNoLocation,
                          Piece::kNoPiece, QUEEN));

  move_or = ParseMove(*board, "h10h11R");
  EXPECT_EQ(move_or, Move(BoardLocation(4, 7), BoardLocation(3, 7),
                          Piece::kNoPiece, BoardLocation::kNoLocation,
                          Piece::kNoPiece, ROOK));

  move_or = ParseMove(*board, "h10h11B");
  EXPECT_EQ(move_or, Move(BoardLocation(4, 7), BoardLocation(3, 7),
                          Piece::kNoPiece, BoardLocation::kNoLocation,
                          Piece::kNoPiece, BISHOP));

  move_or = ParseMove(*board, "h10h11N");
  EXPECT_EQ(move_or, Move(BoardLocation(4, 7), BoardLocation(3, 7),
                          Piece::kNoPiece, BoardLocation::kNoLocation,
                          Piece::kNoPiece, KNIGHT));

  // with '='
  move_or = ParseMove(*board, "h10h11=q");
  EXPECT_EQ(move_or, Move(BoardLocation(4, 7), BoardLocation(3, 7),
                          Piece::kNoPiece, BoardLocation::kNoLocation,
                          Piece::kNoPiece, QUEEN));

  // capture
  move_or = ParseMove(*board, "h10xg11=q");
  EXPECT_EQ(move_or, Move(BoardLocation(4, 7), BoardLocation(3, 6),
                          board->GetPiece(3, 6), BoardLocation::kNoLocation,
                          Piece::kNoPiece, QUEEN));

  move_or = ParseMove(*board, "h10xi11=r");
  EXPECT_EQ(move_or, Move(BoardLocation(4, 7), BoardLocation(3, 8),
                          board->GetPiece(3, 8), BoardLocation::kNoLocation,
                          Piece::kNoPiece, ROOK));
}


}  // namespace chess


