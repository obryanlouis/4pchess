
#include <unordered_map>
#include <vector>
#include <gtest/gtest.h>
#include "gmock/gmock.h"

#include "board.h"
#include "utils.h"

namespace chess {

using Loc = BoardLocation;
using ::testing::UnorderedElementsAre;

namespace {

std::vector<Move> GetMoves(Board& board, PieceType piece_type) {
  std::vector<Move> moves;

  Move move_buffer[300];
  size_t num_moves = board.GetPseudoLegalMoves2(move_buffer, 300);
  for (size_t i = 0; i < num_moves && i < 300; i++) {
    const auto& move = move_buffer[i];
    const auto& piece = board.GetPiece(move.From());
    if (piece.GetPieceType() == piece_type) {
      moves.push_back(move);
    }
  }
  return moves;
}

}  // namespace

TEST(BoardLocationTest, Properties) {
  BoardLocation x(0, 0);
  BoardLocation y(1, 2);
  BoardLocation z(1, 2);

  EXPECT_NE(x, y);
  EXPECT_EQ(y, z);
  EXPECT_EQ(x.GetRow(), 0);
  EXPECT_EQ(x.GetCol(), 0);
  EXPECT_EQ(y.GetRow(), 1);
  EXPECT_EQ(y.GetCol(), 2);
}

TEST(PlayerTest, Properties) {
  Player red(RED);
  Player blue(BLUE);
  Player red2(RED);

  EXPECT_EQ(red.GetColor(), RED);
  EXPECT_EQ(red.GetTeam(), RED_YELLOW);
  EXPECT_EQ(blue.GetColor(), BLUE);
  EXPECT_EQ(blue.GetTeam(), BLUE_GREEN);

  EXPECT_EQ(red, red2);
  EXPECT_NE(red, blue);
}

//TEST(BoardTest, GetLegalMoves_Pawn) {
//  // Move (allowed, blocked, outside board bounds, initial)
//  // En-passant (same/other team, non/last move, locations, moved only once)
//  // Non-enpassant capture (same/other team, location of piece, location in
//  //                        bounds)
//  // Promotion (rank, piece types)
//  // Combinations of the above (move + promotion, en-passant + promotion, etc.)
//  // Pawns of color move in correct direction
//
//  std::unordered_map<BoardLocation, Piece> location_to_piece;
//
//  Board board(kRedPlayer, location_to_piece);
//  std::vector<Move> moves;
//
//  Piece red_pawn(kRedPlayer, PAWN);
//  Piece blue_pawn(kBluePlayer, PAWN);
//  Piece yellow_pawn(kYellowPlayer, PAWN);
//  Piece green_pawn(kGreenPlayer, PAWN);
//
//  // Red
//
//  moves.clear();
//  board.GetPawnMoves(moves, BoardLocation(12, 3), red_pawn);
//  EXPECT_THAT(
//      moves,
//      UnorderedElementsAre(
//        // Move forward
//        Move(BoardLocation(12, 3), BoardLocation(11, 3)),
//        // Move forward 2
//        Move(BoardLocation(12, 3), BoardLocation(10, 3))));
//
//  moves.clear();
//  board.GetPawnMoves(moves, BoardLocation(11, 3), red_pawn);
//  EXPECT_THAT(
//      moves,
//      UnorderedElementsAre(
//        // Move forward
//        Move(BoardLocation(11, 3), BoardLocation(10, 3))));
//
//  // Blue
//
//  moves.clear();
//  board.GetPawnMoves(moves, BoardLocation(3, 1), blue_pawn);
//  EXPECT_THAT(
//      moves,
//      UnorderedElementsAre(
//        // Move forward
//        Move(BoardLocation(3, 1), BoardLocation(3, 2)),
//        // Move forward 2
//        Move(BoardLocation(3, 1), BoardLocation(3, 3))));
//
//  moves.clear();
//  board.GetPawnMoves(moves, BoardLocation(3, 2), blue_pawn);
//  EXPECT_THAT(
//      moves,
//      UnorderedElementsAre(
//        // Move forward
//        Move(BoardLocation(3, 2), BoardLocation(3, 3))));
//
//  // Yellow
//
//  moves.clear();
//  board.GetPawnMoves(moves, BoardLocation(1, 3), yellow_pawn);
//  EXPECT_THAT(
//      moves,
//      UnorderedElementsAre(
//        // Move forward
//        Move(BoardLocation(1, 3), BoardLocation(2, 3)),
//        // Move forward 2
//        Move(BoardLocation(1, 3), BoardLocation(3, 3))));
//
//  moves.clear();
//  board.GetPawnMoves(moves, BoardLocation(2, 3), yellow_pawn);
//  EXPECT_THAT(
//      moves,
//      UnorderedElementsAre(
//        // Move forward
//        Move(BoardLocation(2, 3), BoardLocation(3, 3))));
//
//  // Green
//
//  moves.clear();
//  board.GetPawnMoves(moves, BoardLocation(3, 12), green_pawn);
//  EXPECT_THAT(
//      moves,
//      UnorderedElementsAre(
//        // Move forward
//        Move(BoardLocation(3, 12), BoardLocation(3, 11)),
//        // Move forward 2
//        Move(BoardLocation(3, 12), BoardLocation(3, 10))));
//
//  moves.clear();
//  board.GetPawnMoves(moves, BoardLocation(3, 11), green_pawn);
//  EXPECT_THAT(
//      moves,
//      UnorderedElementsAre(
//        // Move forward
//        Move(BoardLocation(3, 11), BoardLocation(3, 10))));
//
//  // En-passant
//
//  location_to_piece = {
//    {BoardLocation(12, 4), red_pawn},
//    {BoardLocation(10, 3), blue_pawn},
//    {BoardLocation(10, 5), green_pawn},
//  };
//  board = Board(kRedPlayer, location_to_piece);
//
//  // Advance red pawn twice
//  board.MakeMove(Move(BoardLocation(12, 4), BoardLocation(10, 4)));
//  EXPECT_TRUE(board.GetPiece(BoardLocation(10, 4)).has_value());
//
//  moves.clear();
//  board.GetPawnMoves(moves, BoardLocation(10, 3), blue_pawn);
//  EXPECT_THAT(
//      moves,
//      UnorderedElementsAre(
//        // En-passant
//        Move(BoardLocation(10, 3), BoardLocation(11, 4),
//             /*standard_capture=*/std::nullopt,
//             /*en_passant_location=*/BoardLocation(10, 4),
//             /*en_passant_capture=*/red_pawn,
//             /*promotion_piece_type=*/std::nullopt)));
//
//
////  player_to_pieces = {
////    {kRedPlayer,
////      {
////        PlacedPiece(BoardLocation(10, 4), Piece(kRedPlayer, PAWN)),
////      }},
////    {kBluePlayer,
////      {
////        PlacedPiece(BoardLocation(10, 3), Piece(kBluePlayer, PAWN)),
////      }},
////    {kYellowPlayer, {}},
////    {kGreenPlayer,
////      {
////        PlacedPiece(BoardLocation(10, 5), Piece(kGreenPlayer, PAWN)),
////      }},
////  };
////  Move last_move(
////      BoardLocation(12, 4), BoardLocation(10, 4),
////      Piece(kRedPlayer, PAWN));
////
////  board = Board(kRedPlayer, player_to_pieces, last_move);
//
////  moves = board.GetPawnMoves(
////      PlacedPiece(BoardLocation(10, 3), Piece(kBluePlayer, PAWN)));
////  EXPECT_THAT(
////      moves,
////      UnorderedElementsAre(
////        // En-passant
////        Move(BoardLocation(10, 3), BoardLocation(11, 4),
////             Piece(kBluePlayer, PAWN), /*promotion=*/std::nullopt,
////             BoardLocation(10, 4))));
////
////  moves = board.GetPawnMoves(
////      PlacedPiece(BoardLocation(10, 5), Piece(kGreenPlayer, PAWN)));
////  EXPECT_THAT(
////      moves,
////      UnorderedElementsAre(
////        // En-passant
////        Move(BoardLocation(10, 5), BoardLocation(11, 4),
////             Piece(kGreenPlayer, PAWN), /*promotion=*/std::nullopt,
////             BoardLocation(10, 4))));
////
////  // Last move not to square => no en-passant.
////  // This also tests the case that a pawn move is blocked by another piece.
////  board = Board(kRedPlayer, player_to_pieces);
////  moves = board.GetPawnMoves(
////      PlacedPiece(BoardLocation(10, 3), Piece(kBluePlayer, PAWN)));
////  EXPECT_EQ(moves.size(), 0);
////
////  // Non-enpassant capture
////
////  player_to_pieces = {
////    {kRedPlayer,
////      {
////        PlacedPiece(BoardLocation(10, 4), Piece(kRedPlayer, PAWN)),
////        PlacedPiece(BoardLocation(9, 4), Piece(kRedPlayer, KING)),
////      }},
////    {kBluePlayer,
////      {
////        PlacedPiece(BoardLocation(9, 3), Piece(kBluePlayer, PAWN)),
////      }},
////    {kYellowPlayer,
////      {
////        PlacedPiece(BoardLocation(8, 4), Piece(kYellowPlayer, PAWN)),
////      }},
////    {kGreenPlayer,
////      {
////        PlacedPiece(BoardLocation(9, 5), Piece(kGreenPlayer, PAWN)),
////      }},
////  };
////
////  board = Board(kRedPlayer, player_to_pieces);
////
////  moves = board.GetPawnMoves(
////      PlacedPiece(BoardLocation(10, 4), Piece(kRedPlayer, PAWN)));
////  EXPECT_THAT(
////      moves,
////      UnorderedElementsAre(
////        Move(BoardLocation(10, 4), BoardLocation(9, 3), Piece(kRedPlayer, PAWN)),
////        Move(BoardLocation(10, 4), BoardLocation(9, 5), Piece(kRedPlayer, PAWN))));
////
////  moves = board.GetPawnMoves(
////      PlacedPiece(BoardLocation(9, 3), Piece(kBluePlayer, PAWN)));
////  EXPECT_THAT(
////      moves,
////      UnorderedElementsAre(
////        Move(BoardLocation(9, 3), BoardLocation(8, 4), Piece(kBluePlayer, PAWN)),
////        Move(BoardLocation(9, 3), BoardLocation(10, 4), Piece(kBluePlayer, PAWN))));
////
////  moves = board.GetPawnMoves(
////      PlacedPiece(BoardLocation(8, 4), Piece(kYellowPlayer, PAWN)));
////  EXPECT_THAT(
////      moves,
////      UnorderedElementsAre(
////        Move(BoardLocation(8, 4), BoardLocation(9, 3), Piece(kYellowPlayer, PAWN)),
////        Move(BoardLocation(8, 4), BoardLocation(9, 5), Piece(kYellowPlayer, PAWN))));
////
////  moves = board.GetPawnMoves(
////      PlacedPiece(BoardLocation(9, 5), Piece(kGreenPlayer, PAWN)));
////  EXPECT_THAT(
////      moves,
////      UnorderedElementsAre(
////        Move(BoardLocation(9, 5), BoardLocation(8, 4), Piece(kGreenPlayer, PAWN)),
////        Move(BoardLocation(9, 5), BoardLocation(10, 4), Piece(kGreenPlayer, PAWN))));
////
////  // No capturing same team
////
////  player_to_pieces = {
////    {kRedPlayer,
////      {
////        PlacedPiece(BoardLocation(10, 4), Piece(kRedPlayer, PAWN)),
////        PlacedPiece(BoardLocation(9, 4), Piece(kRedPlayer, KING)),
////      }},
////    {kBluePlayer, {}},
////    {kYellowPlayer,
////      {
////        PlacedPiece(BoardLocation(9, 3), Piece(kYellowPlayer, PAWN)),
////      }},
////    {kGreenPlayer, {}},
////  };
////
////  board = Board(kRedPlayer, player_to_pieces);
////  moves = board.GetPawnMoves(
////      PlacedPiece(BoardLocation(10, 4), Piece(kRedPlayer, PAWN)));
////  EXPECT_THAT(moves, UnorderedElementsAre());
////
////  // Promotion
////
////  player_to_pieces = {
////    {kRedPlayer,
////      {
////        PlacedPiece(BoardLocation(4, 3), Piece(kRedPlayer, PAWN)),
////      }},
////    {kBluePlayer,
////      {
////        PlacedPiece(BoardLocation(3, 9), Piece(kBluePlayer, PAWN)),
////      }},
////    {kYellowPlayer,
////      {
////        PlacedPiece(BoardLocation(9, 3), Piece(kYellowPlayer, PAWN)),
////      }},
////    {kGreenPlayer,
////      {
////        PlacedPiece(BoardLocation(7, 4), Piece(kGreenPlayer, PAWN)),
////      }},
////  };
////  board = Board(kRedPlayer, player_to_pieces);
////
////  moves = board.GetPawnMoves(
////      PlacedPiece(BoardLocation(4, 3), Piece(kRedPlayer, PAWN)));
////  EXPECT_THAT(
////      moves,
////      UnorderedElementsAre(
////        Move(BoardLocation(4, 3), BoardLocation(3, 3), Piece(kRedPlayer, PAWN),
////             KNIGHT),
////        Move(BoardLocation(4, 3), BoardLocation(3, 3), Piece(kRedPlayer, PAWN),
////             BISHOP),
////        Move(BoardLocation(4, 3), BoardLocation(3, 3), Piece(kRedPlayer, PAWN),
////             ROOK),
////        Move(BoardLocation(4, 3), BoardLocation(3, 3), Piece(kRedPlayer, PAWN),
////             QUEEN)));
////
////  moves = board.GetPawnMoves(
////      PlacedPiece(BoardLocation(3, 9), Piece(kBluePlayer, PAWN)));
////  EXPECT_THAT(
////      moves,
////      UnorderedElementsAre(
////        Move(BoardLocation(3, 9), BoardLocation(3, 10), Piece(kBluePlayer, PAWN),
////             KNIGHT),
////        Move(BoardLocation(3, 9), BoardLocation(3, 10), Piece(kBluePlayer, PAWN),
////             BISHOP),
////        Move(BoardLocation(3, 9), BoardLocation(3, 10), Piece(kBluePlayer, PAWN),
////             ROOK),
////        Move(BoardLocation(3, 9), BoardLocation(3, 10), Piece(kBluePlayer, PAWN),
////             QUEEN)));
////
////  moves = board.GetPawnMoves(
////      PlacedPiece(BoardLocation(9, 3), Piece(kYellowPlayer, PAWN)));
////  EXPECT_THAT(
////      moves,
////      UnorderedElementsAre(
////        Move(BoardLocation(9, 3), BoardLocation(10, 3), Piece(kYellowPlayer, PAWN),
////             KNIGHT),
////        Move(BoardLocation(9, 3), BoardLocation(10, 3), Piece(kYellowPlayer, PAWN),
////             BISHOP),
////        Move(BoardLocation(9, 3), BoardLocation(10, 3), Piece(kYellowPlayer, PAWN),
////             ROOK),
////        Move(BoardLocation(9, 3), BoardLocation(10, 3), Piece(kYellowPlayer, PAWN),
////             QUEEN)));
////
////  moves = board.GetPawnMoves(
////      PlacedPiece(BoardLocation(7, 4), Piece(kGreenPlayer, PAWN)));
////  EXPECT_THAT(
////      moves,
////      UnorderedElementsAre(
////        Move(BoardLocation(7, 4), BoardLocation(7, 3), Piece(kGreenPlayer, PAWN),
////             KNIGHT),
////        Move(BoardLocation(7, 4), BoardLocation(7, 3), Piece(kGreenPlayer, PAWN),
////             BISHOP),
////        Move(BoardLocation(7, 4), BoardLocation(7, 3), Piece(kGreenPlayer, PAWN),
////             ROOK),
////        Move(BoardLocation(7, 4), BoardLocation(7, 3), Piece(kGreenPlayer, PAWN),
////             QUEEN)));
////
////  // Capture + promote
////
////  player_to_pieces = {
////    {kRedPlayer,
////      {
////        PlacedPiece(BoardLocation(4, 3), Piece(kRedPlayer, PAWN)),
////        PlacedPiece(BoardLocation(3, 3), Piece(kRedPlayer, KING)),
////      }},
////    {kBluePlayer,
////      {
////        PlacedPiece(BoardLocation(3, 2), Piece(kBluePlayer, PAWN)),
////      }},
////    {kYellowPlayer, {}},
////    {kGreenPlayer, {}},
////  };
////  board = Board(kRedPlayer, player_to_pieces);
////
////  moves = board.GetPawnMoves(
////      PlacedPiece(BoardLocation(4, 3), Piece(kRedPlayer, PAWN)));
////  EXPECT_THAT(
////      moves,
////      UnorderedElementsAre(
////        Move(BoardLocation(4, 3), BoardLocation(3, 2), Piece(kRedPlayer, PAWN),
////             KNIGHT),
////        Move(BoardLocation(4, 3), BoardLocation(3, 2), Piece(kRedPlayer, PAWN),
////             BISHOP),
////        Move(BoardLocation(4, 3), BoardLocation(3, 2), Piece(kRedPlayer, PAWN),
////             ROOK),
////        Move(BoardLocation(4, 3), BoardLocation(3, 2), Piece(kRedPlayer, PAWN),
////             QUEEN)));
//
//}

//TEST(BoardTest, GetLegalMoves_Knight) {
//  // Move (allowed, outside board bounds)
//  // Capture (same/other team, location of piece, location in bounds)
//
//  std::unordered_map<Player, std::vector<PlacedPiece>> player_to_pieces = {
//    {kRedPlayer,
//      {
//        PlacedPiece(BoardLocation(7, 7), Piece(kRedPlayer, KNIGHT)),
//      }},
//    {kBluePlayer,
//      {
//        PlacedPiece(BoardLocation(5, 6), Piece(kBluePlayer, KNIGHT)),
//        PlacedPiece(BoardLocation(5, 8), Piece(kBluePlayer, KNIGHT)),
//        PlacedPiece(BoardLocation(9, 6), Piece(kBluePlayer, KNIGHT)),
//        PlacedPiece(BoardLocation(9, 8), Piece(kBluePlayer, KNIGHT)),
//      }},
//    {kYellowPlayer,
//      {
//        PlacedPiece(BoardLocation(6, 5), Piece(kYellowPlayer, KNIGHT)),
//        PlacedPiece(BoardLocation(6, 9), Piece(kYellowPlayer, BISHOP)),
//      }},
//    {kGreenPlayer,
//      {
//        PlacedPiece(BoardLocation(8, 9), Piece(kGreenPlayer, BISHOP)),
//      }},
//  };
//
//  Board board(kRedPlayer, player_to_pieces);
//  std::vector<Move> moves;
//
//  moves = board.GetKnightMoves(
//      PlacedPiece(BoardLocation(7, 7), Piece(kRedPlayer, KNIGHT)));
//  EXPECT_THAT(
//      moves,
//      UnorderedElementsAre(
//        Move(BoardLocation(7, 7), BoardLocation(5, 6), Piece(kRedPlayer, KNIGHT)),
//        Move(BoardLocation(7, 7), BoardLocation(5, 8), Piece(kRedPlayer, KNIGHT)),
//        Move(BoardLocation(7, 7), BoardLocation(9, 6), Piece(kRedPlayer, KNIGHT)),
//        Move(BoardLocation(7, 7), BoardLocation(9, 8), Piece(kRedPlayer, KNIGHT)),
//        Move(BoardLocation(7, 7), BoardLocation(8, 5), Piece(kRedPlayer, KNIGHT)),
//        Move(BoardLocation(7, 7), BoardLocation(8, 9), Piece(kRedPlayer, KNIGHT))));
//
//}

//TEST(BoardTest, GetLegalMoves_Bishop) {
//  // Move (allowed, blocked, outside board bounds)
//  // Capture (same/other team, location of piece, location in bounds)
//
//  Piece red_bishop(kRedPlayer, BISHOP);
//  Piece blue_bishop(kBluePlayer, BISHOP);
//  Piece yellow_bishop(kYellowPlayer, BISHOP);
//  Piece green_bishop(kGreenPlayer, BISHOP);
//
//  std::unordered_map<BoardLocation, Piece> location_to_piece = {
//    {BoardLocation(7, 7), red_bishop},
//    {BoardLocation(6, 6), blue_bishop},
//    {BoardLocation(9, 9), yellow_bishop},
//    {BoardLocation(9, 5), green_bishop},
//  };
//
//  Board board(kRedPlayer, location_to_piece);
//  std::vector<Move> moves;
//
//  board.GetBishopMoves(moves, BoardLocation(7, 7), red_bishop);
//
//  EXPECT_THAT(
//      moves,
//      UnorderedElementsAre(
//        Move(BoardLocation(7, 7), BoardLocation(6, 6), blue_bishop),
//        Move(BoardLocation(7, 7), BoardLocation(8, 8)),
//        Move(BoardLocation(7, 7), BoardLocation(8, 6)),
//        Move(BoardLocation(7, 7), BoardLocation(9, 5), green_bishop),
//        Move(BoardLocation(7, 7), BoardLocation(6, 8)),
//        Move(BoardLocation(7, 7), BoardLocation(5, 9)),
//        Move(BoardLocation(7, 7), BoardLocation(4, 10)),
//        Move(BoardLocation(7, 7), BoardLocation(3, 11))));
//
//}

//TEST(BoardTest, GetLegalMoves_Rook) {
//  // Move (allowed, blocked, outside board bounds)
//  // Capture (same/other team, location of piece, location in bounds)
//
//  std::unordered_map<Player, std::vector<PlacedPiece>> player_to_pieces = {
//    {kRedPlayer,
//      {
//        PlacedPiece(BoardLocation(7, 7), Piece(kRedPlayer, ROOK)),
//      }},
//    {kBluePlayer,
//      {
//        PlacedPiece(BoardLocation(7, 3), Piece(kBluePlayer, KNIGHT)),
//      }},
//    {kYellowPlayer,
//      {
//        PlacedPiece(BoardLocation(7, 10), Piece(kYellowPlayer, KNIGHT)),
//      }},
//    {kGreenPlayer,
//      {
//        PlacedPiece(BoardLocation(2, 7), Piece(kGreenPlayer, BISHOP)),
//      }},
//  };
//
//  Board board(kRedPlayer, player_to_pieces);
//  std::vector<Move> moves;
//
//  moves = board.GetRookMoves(
//      PlacedPiece(BoardLocation(7, 7), Piece(kRedPlayer, ROOK)));
//
//  EXPECT_THAT(
//      moves,
//      UnorderedElementsAre(
//        Move(BoardLocation(7, 7), BoardLocation(6, 7), Piece(kRedPlayer, ROOK)),
//        Move(BoardLocation(7, 7), BoardLocation(5, 7), Piece(kRedPlayer, ROOK)),
//        Move(BoardLocation(7, 7), BoardLocation(4, 7), Piece(kRedPlayer, ROOK)),
//        Move(BoardLocation(7, 7), BoardLocation(3, 7), Piece(kRedPlayer, ROOK)),
//        Move(BoardLocation(7, 7), BoardLocation(2, 7), Piece(kRedPlayer, ROOK)),
//        Move(BoardLocation(7, 7), BoardLocation(8, 7), Piece(kRedPlayer, ROOK)),
//        Move(BoardLocation(7, 7), BoardLocation(9, 7), Piece(kRedPlayer, ROOK)),
//        Move(BoardLocation(7, 7), BoardLocation(10, 7), Piece(kRedPlayer, ROOK)),
//        Move(BoardLocation(7, 7), BoardLocation(11, 7), Piece(kRedPlayer, ROOK)),
//        Move(BoardLocation(7, 7), BoardLocation(12, 7), Piece(kRedPlayer, ROOK)),
//        Move(BoardLocation(7, 7), BoardLocation(13, 7), Piece(kRedPlayer, ROOK)),
//        Move(BoardLocation(7, 7), BoardLocation(7, 6), Piece(kRedPlayer, ROOK)),
//        Move(BoardLocation(7, 7), BoardLocation(7, 5), Piece(kRedPlayer, ROOK)),
//        Move(BoardLocation(7, 7), BoardLocation(7, 4), Piece(kRedPlayer, ROOK)),
//        Move(BoardLocation(7, 7), BoardLocation(7, 3), Piece(kRedPlayer, ROOK)),
//        Move(BoardLocation(7, 7), BoardLocation(7, 8), Piece(kRedPlayer, ROOK)),
//        Move(BoardLocation(7, 7), BoardLocation(7, 9), Piece(kRedPlayer, ROOK))));
//
//}
//
//TEST(BoardTest, GetLegalMoves_Queen) {
//  // Move (allowed, blocked, outside board bounds)
//  // Capture (same/other team, location of piece, location in bounds)
//
//  std::unordered_map<Player, std::vector<PlacedPiece>> player_to_pieces = {
//    {kRedPlayer,
//      {
//        PlacedPiece(BoardLocation(7, 7), Piece(kRedPlayer, QUEEN)),
//      }},
//    {kBluePlayer,
//      {
//        PlacedPiece(BoardLocation(7, 3), Piece(kBluePlayer, KNIGHT)),
//        PlacedPiece(BoardLocation(5, 5), Piece(kBluePlayer, KNIGHT)),
//      }},
//    {kYellowPlayer,
//      {
//        PlacedPiece(BoardLocation(7, 10), Piece(kYellowPlayer, KNIGHT)),
//        PlacedPiece(BoardLocation(9, 9), Piece(kYellowPlayer, KNIGHT)),
//      }},
//    {kGreenPlayer,
//      {
//        PlacedPiece(BoardLocation(2, 7), Piece(kGreenPlayer, BISHOP)),
//      }},
//  };
//
//  Board board(kRedPlayer, player_to_pieces);
//  std::vector<Move> moves;
//
//  moves = board.GetQueenMoves(
//      PlacedPiece(BoardLocation(7, 7), Piece(kRedPlayer, QUEEN)));
//
//  EXPECT_THAT(
//      moves,
//      UnorderedElementsAre(
//        // Rook moves
//        Move(BoardLocation(7, 7), BoardLocation(6, 7), Piece(kRedPlayer, QUEEN)),
//        Move(BoardLocation(7, 7), BoardLocation(5, 7), Piece(kRedPlayer, QUEEN)),
//        Move(BoardLocation(7, 7), BoardLocation(4, 7), Piece(kRedPlayer, QUEEN)),
//        Move(BoardLocation(7, 7), BoardLocation(3, 7), Piece(kRedPlayer, QUEEN)),
//        Move(BoardLocation(7, 7), BoardLocation(2, 7), Piece(kRedPlayer, QUEEN)),
//        Move(BoardLocation(7, 7), BoardLocation(8, 7), Piece(kRedPlayer, QUEEN)),
//        Move(BoardLocation(7, 7), BoardLocation(9, 7), Piece(kRedPlayer, QUEEN)),
//        Move(BoardLocation(7, 7), BoardLocation(10, 7), Piece(kRedPlayer, QUEEN)),
//        Move(BoardLocation(7, 7), BoardLocation(11, 7), Piece(kRedPlayer, QUEEN)),
//        Move(BoardLocation(7, 7), BoardLocation(12, 7), Piece(kRedPlayer, QUEEN)),
//        Move(BoardLocation(7, 7), BoardLocation(13, 7), Piece(kRedPlayer, QUEEN)),
//        Move(BoardLocation(7, 7), BoardLocation(7, 6), Piece(kRedPlayer, QUEEN)),
//        Move(BoardLocation(7, 7), BoardLocation(7, 5), Piece(kRedPlayer, QUEEN)),
//        Move(BoardLocation(7, 7), BoardLocation(7, 4), Piece(kRedPlayer, QUEEN)),
//        Move(BoardLocation(7, 7), BoardLocation(7, 3), Piece(kRedPlayer, QUEEN)),
//        Move(BoardLocation(7, 7), BoardLocation(7, 8), Piece(kRedPlayer, QUEEN)),
//        Move(BoardLocation(7, 7), BoardLocation(7, 9), Piece(kRedPlayer, QUEEN)),
//        // Bishop moves
//        Move(BoardLocation(7, 7), BoardLocation(6, 6), Piece(kRedPlayer, QUEEN)),
//        Move(BoardLocation(7, 7), BoardLocation(5, 5), Piece(kRedPlayer, QUEEN)),
//        Move(BoardLocation(7, 7), BoardLocation(8, 8), Piece(kRedPlayer, QUEEN)),
//        Move(BoardLocation(7, 7), BoardLocation(6, 8), Piece(kRedPlayer, QUEEN)),
//        Move(BoardLocation(7, 7), BoardLocation(5, 9), Piece(kRedPlayer, QUEEN)),
//        Move(BoardLocation(7, 7), BoardLocation(4, 10), Piece(kRedPlayer, QUEEN)),
//        Move(BoardLocation(7, 7), BoardLocation(3, 11), Piece(kRedPlayer, QUEEN)),
//        Move(BoardLocation(7, 7), BoardLocation(8, 6), Piece(kRedPlayer, QUEEN)),
//        Move(BoardLocation(7, 7), BoardLocation(9, 5), Piece(kRedPlayer, QUEEN)),
//        Move(BoardLocation(7, 7), BoardLocation(10, 4), Piece(kRedPlayer, QUEEN)),
//        Move(BoardLocation(7, 7), BoardLocation(11, 3), Piece(kRedPlayer, QUEEN))));
//
//}

TEST(BoardTest, GetLegalMoves_King) {
  std::shared_ptr<Board> board;
  std::vector<Move> moves;

  // normal move -- all directions
  board = ParseBoardFromFEN("R-0,0,0,0-1,1,1,1-1,1,1,1-0,0,0,0-0-x,x,x,yR,yN,yB,yK,yQ,yB,yN,yR,x,x,x/x,x,x,yP,yP,yP,yP,yP,yP,yP,yP,x,x,x/x,x,x,8,x,x,x/bR,bP,10,gP,gR/bN,bP,10,gP,gN/bB,bP,10,gP,gB/bQ,bP,10,gP,gK/bK,bP,10,gP,gQ/bB,bP,10,gP,gB/bN,bP,5,rK,4,gP,gN/bR,bP,10,gP,gR/x,x,x,8,x,x,x/x,x,x,rP,rP,rP,rP,rP,rP,rP,rP,x,x,x/x,x,x,rR,rN,rB,rQ,1,rB,rN,rR,x,x,x");
  ASSERT_NE(board, nullptr);

  moves = GetMoves(*board, KING);
  EXPECT_THAT(
      moves,
      UnorderedElementsAre(
        ParseMove(*board, "h5-i4"),
        ParseMove(*board, "h5-i5"),
        ParseMove(*board, "h5-i6"),
        ParseMove(*board, "h5-h4"),
        ParseMove(*board, "h5-h6"),
        ParseMove(*board, "h5-g4"),
        ParseMove(*board, "h5-g5"),
        ParseMove(*board, "h5-g6")));

  // can't move into check

  // castling: kingside & queenside
  board = ParseBoardFromFEN("R-0,0,0,0-1,1,1,1-1,1,1,1-0,0,0,0-0-x,x,x,yR,2,yK,3,yR,x,x,x/x,x,x,yP,yP,yP,yP,yP,yP,yP,yP,x,x,x/x,x,x,8,x,x,x/bR,bP,10,gP,gR/1,bP,10,gP,1/1,bP,10,gP,1/1,bP,10,gP,gK/bK,bP,10,gP,1/1,bP,10,gP,1/1,bP,10,gP,1/bR,bP,10,gP,gR/x,x,x,8,x,x,x/x,x,x,rP,rP,rP,rP,rP,rP,rP,rP,x,x,x/x,x,x,rR,3,rK,2,rR,x,x,x");
  moves = GetMoves(*board, KING);
  EXPECT_THAT(
      moves,
      UnorderedElementsAre(
        ParseMove(*board, "h1-i1"),
        ParseMove(*board, "h1-j1"),
        ParseMove(*board, "h1-g1"),
        ParseMove(*board, "h1-f1")));

  // castling: all colors
  board = ParseBoardFromFEN("B-0,0,0,0-1,1,1,1-1,1,1,1-0,0,0,0-0-x,x,x,yR,2,yK,3,yR,x,x,x/x,x,x,yP,yP,yP,yP,yP,yP,yP,yP,x,x,x/x,x,x,8,x,x,x/bR,bP,10,gP,gR/1,bP,10,gP,1/1,bP,10,gP,1/1,bP,10,gP,gK/bK,bP,10,gP,1/1,bP,10,gP,1/1,bP,10,gP,1/bR,bP,10,gP,gR/x,x,x,8,x,x,x/x,x,x,rP,rP,rP,rP,rP,rP,rP,rP,x,x,x/x,x,x,rR,3,rK,2,rR,x,x,x");
  moves = GetMoves(*board, KING);
  EXPECT_THAT(
      moves,
      UnorderedElementsAre(
        ParseMove(*board, "a7-a5"),
        ParseMove(*board, "a7-a6"),
        ParseMove(*board, "a7-a8"),
        ParseMove(*board, "a7-a9")));

  board = ParseBoardFromFEN("Y-0,0,0,0-1,1,1,1-1,1,1,1-0,0,0,0-0-x,x,x,yR,2,yK,3,yR,x,x,x/x,x,x,yP,yP,yP,yP,yP,yP,yP,yP,x,x,x/x,x,x,8,x,x,x/bR,bP,10,gP,gR/1,bP,10,gP,1/1,bP,10,gP,1/1,bP,10,gP,gK/bK,bP,10,gP,1/1,bP,10,gP,1/1,bP,10,gP,1/bR,bP,10,gP,gR/x,x,x,8,x,x,x/x,x,x,rP,rP,rP,rP,rP,rP,rP,rP,x,x,x/x,x,x,rR,3,rK,2,rR,x,x,x");
  moves = GetMoves(*board, KING);
  EXPECT_THAT(
      moves,
      UnorderedElementsAre(
        ParseMove(*board, "g14-e14"),
        ParseMove(*board, "g14-f14"),
        ParseMove(*board, "g14-h14"),
        ParseMove(*board, "g14-i14")));

  board = ParseBoardFromFEN("G-0,0,0,0-1,1,1,1-1,1,1,1-0,0,0,0-0-x,x,x,yR,2,yK,3,yR,x,x,x/x,x,x,yP,yP,yP,yP,yP,yP,yP,yP,x,x,x/x,x,x,8,x,x,x/bR,bP,10,gP,gR/1,bP,10,gP,1/1,bP,10,gP,1/1,bP,10,gP,gK/bK,bP,10,gP,1/1,bP,10,gP,1/1,bP,10,gP,1/bR,bP,10,gP,gR/x,x,x,8,x,x,x/x,x,x,rP,rP,rP,rP,rP,rP,rP,rP,x,x,x/x,x,x,rR,3,rK,2,rR,x,x,x");
  moves = GetMoves(*board, KING);
  EXPECT_THAT(
      moves,
      UnorderedElementsAre(
        ParseMove(*board, "n8-n6"),
        ParseMove(*board, "n8-n7"),
        ParseMove(*board, "n8-n9"),
        ParseMove(*board, "n8-n10")));

  // castling through check not allowed
  board = ParseBoardFromFEN("R-0,0,0,0-1,1,1,1-1,1,1,1-0,0,0,0-0-x,x,x,yR,2,yK,3,yR,x,x,x/x,x,x,yP,yP,yP,yP,yP,yP,yP,yP,x,x,x/x,x,x,8,x,x,x/bR,bP,10,gP,gR/1,bP,10,gP,1/1,bP,10,gP,1/1,bP,4,bR,5,gP,gK/bK,bP,10,gP,1/1,bP,10,gP,1/1,bP,10,gP,1/bR,bP,10,gP,gR/x,x,x,8,x,x,x/x,x,x,rP,rP,rP,1,rP,rP,rP,rP,x,x,x/x,x,x,rR,3,rK,2,rR,x,x,x");
  moves = GetMoves(*board, KING);
  EXPECT_THAT(
      moves,
      UnorderedElementsAre(
        ParseMove(*board, "h1-i1"),
        ParseMove(*board, "h1-j1"),
        // pseudo legal moves into check
        ParseMove(*board, "h1-g1"),
        ParseMove(*board, "h1-g2")));

  // castling not allowed while in check

  board = ParseBoardFromFEN("R-0,0,0,0-1,1,1,1-1,1,1,1-0,0,0,0-0-x,x,x,yR,2,yK,3,yR,x,x,x/x,x,x,yP,yP,yP,yP,yP,yP,yP,yP,x,x,x/x,x,x,8,x,x,x/bR,bP,10,gP,gR/1,bP,10,gP,1/1,bP,10,gP,1/1,bP,4,bR,5,gP,gK/bK,bP,10,gP,1/1,bP,10,gP,1/1,bP,1,bB,8,gP,1/bR,bP,10,gP,gR/x,x,x,8,x,x,x/x,x,x,rP,rP,rP,1,rP,rP,rP,rP,x,x,x/x,x,x,rR,3,rK,2,rR,x,x,x");
  moves = GetMoves(*board, KING);
  EXPECT_THAT(
      moves,
      UnorderedElementsAre(
        ParseMove(*board, "h1-i1"),
        // pseudo legal moves into check
        ParseMove(*board, "h1-g1"),
        ParseMove(*board, "h1-g2")));

}

//TEST(BoardTest, GetLegalMoves_King) {
//  // Move (allowed, outside board bounds)
//  // Capture (same/other team, location of piece, location in bounds)
//  // Castle (kingside, queenside, through check, after moved king/rook)
//
//  std::unordered_map<Player, std::vector<PlacedPiece>> player_to_pieces = {
//    {kRedPlayer,
//      {
//        PlacedPiece(BoardLocation(3, 3), Piece(kRedPlayer, KING)),
//      }},
//    {kBluePlayer,
//      {
//        PlacedPiece(BoardLocation(2, 3), Piece(kBluePlayer, KNIGHT)),
//      }},
//    {kYellowPlayer,
//      {
//        PlacedPiece(BoardLocation(4, 4), Piece(kYellowPlayer, KNIGHT)),
//        PlacedPiece(BoardLocation(4, 2), Piece(kYellowPlayer, KNIGHT)),
//      }},
//    {kGreenPlayer,
//      {
//      }},
//  };
//
//  Board board(kRedPlayer, player_to_pieces);
//  std::vector<Move> moves;
//
//  moves = board.GetKingMoves(
//      PlacedPiece(BoardLocation(3, 3), Piece(kRedPlayer, KING)));
//
//  EXPECT_THAT(
//    moves,
//    UnorderedElementsAre(
//      Move(BoardLocation(3, 3), BoardLocation(2, 3), Piece(kRedPlayer, KING)),
//      Move(BoardLocation(3, 3), BoardLocation(2, 4), Piece(kRedPlayer, KING)),
//      Move(BoardLocation(3, 3), BoardLocation(3, 2), Piece(kRedPlayer, KING)),
//      Move(BoardLocation(3, 3), BoardLocation(3, 4), Piece(kRedPlayer, KING)),
//      Move(BoardLocation(3, 3), BoardLocation(4, 3), Piece(kRedPlayer, KING))));
//
//  // Castling
//
//  player_to_pieces = {
//    {kRedPlayer,
//      {
//        PlacedPiece(BoardLocation(13, 7), Piece(kRedPlayer, KING)),
//        PlacedPiece(BoardLocation(13, 3), Piece(kRedPlayer, ROOK)),
//        PlacedPiece(BoardLocation(13, 10), Piece(kRedPlayer, ROOK)),
//        PlacedPiece(BoardLocation(12, 6), Piece(kRedPlayer, PAWN)),
//        PlacedPiece(BoardLocation(12, 7), Piece(kRedPlayer, PAWN)),
//        PlacedPiece(BoardLocation(12, 8), Piece(kRedPlayer, PAWN)),
//      }},
//    {kBluePlayer,
//      {
//      }},
//    {kYellowPlayer,
//      {
//      }},
//    {kGreenPlayer,
//      {
//      }},
//  };
//
//  std::unordered_map<Player, CastlingRights> castling_rights = {
//    {kRedPlayer, CastlingRights()},
//    {kBluePlayer, CastlingRights()},
//    {kYellowPlayer, CastlingRights()},
//    {kGreenPlayer, CastlingRights()},
//  };
//
//  board = Board(kRedPlayer, player_to_pieces, std::nullopt, nullptr,
//      castling_rights);
//
//  moves = board.GetKingMoves(
//      PlacedPiece(BoardLocation(13, 7), Piece(kRedPlayer, KING)));
//
//  EXPECT_THAT(
//    moves,
//    UnorderedElementsAre(
//      Move(BoardLocation(13, 7), BoardLocation(13, 6), Piece(kRedPlayer, KING)),
//      Move(BoardLocation(13, 7), BoardLocation(13, 8), Piece(kRedPlayer, KING)),
//      Move(BoardLocation(13, 7), BoardLocation(13, 5), Piece(kRedPlayer, KING),
//           SimpleMove(BoardLocation(13, 3), BoardLocation(13, 6),
//                      Piece(kRedPlayer, ROOK))),
//      Move(BoardLocation(13, 7), BoardLocation(13, 9), Piece(kRedPlayer, KING),
//           SimpleMove(BoardLocation(13, 10), BoardLocation(13, 8),
//                      Piece(kRedPlayer, ROOK)))));
//
//  // Castling through check
//
//  player_to_pieces = {
//    {kRedPlayer,
//      {
//        PlacedPiece(BoardLocation(13, 7), Piece(kRedPlayer, KING)),
//        PlacedPiece(BoardLocation(13, 3), Piece(kRedPlayer, ROOK)),
//        PlacedPiece(BoardLocation(13, 10), Piece(kRedPlayer, ROOK)),
//      }},
//    {kBluePlayer,
//      {
//        PlacedPiece(BoardLocation(5, 8), Piece(kBluePlayer, ROOK)),
//      }},
//    {kYellowPlayer,
//      {
//      }},
//    {kGreenPlayer,
//      {
//        PlacedPiece(BoardLocation(7, 6), Piece(kGreenPlayer, ROOK)),
//      }},
//  };
//
//  board = Board(kRedPlayer, player_to_pieces, std::nullopt, nullptr,
//      castling_rights);
//
//  moves = board.GetKingMoves(
//      PlacedPiece(BoardLocation(13, 7), Piece(kRedPlayer, KING)));
//
//  EXPECT_THAT(
//    moves,
//    UnorderedElementsAre(
//      // Note that the first 3 moves might not be allowed later.
//      // They are currently filtered out in GetLegalMoves.
//      Move(BoardLocation(13, 7), BoardLocation(12, 6), Piece(kRedPlayer, KING)),
//      Move(BoardLocation(13, 7), BoardLocation(12, 7), Piece(kRedPlayer, KING)),
//      Move(BoardLocation(13, 7), BoardLocation(12, 8), Piece(kRedPlayer, KING)),
//      Move(BoardLocation(13, 7), BoardLocation(13, 6), Piece(kRedPlayer, KING)),
//      Move(BoardLocation(13, 7), BoardLocation(13, 8), Piece(kRedPlayer, KING))));
//
//  // Castling while in check
//
//  player_to_pieces = {
//    {kRedPlayer,
//      {
//        PlacedPiece(BoardLocation(13, 7), Piece(kRedPlayer, KING)),
//        PlacedPiece(BoardLocation(13, 3), Piece(kRedPlayer, ROOK)),
//        PlacedPiece(BoardLocation(13, 10), Piece(kRedPlayer, ROOK)),
//      }},
//    {kBluePlayer,
//      {
//        PlacedPiece(BoardLocation(11, 6), Piece(kBluePlayer, KNIGHT)),
//      }},
//    {kYellowPlayer,
//      {
//      }},
//    {kGreenPlayer,
//      {
//      }},
//  };
//
//  board = Board(kRedPlayer, player_to_pieces, std::nullopt, nullptr,
//      castling_rights);
//
//  moves = board.GetKingMoves(
//      PlacedPiece(BoardLocation(13, 7), Piece(kRedPlayer, KING)));
//
//  EXPECT_THAT(
//    moves,
//    UnorderedElementsAre(
//      // Note that the first 3 moves might not be allowed later.
//      // They are currently filtered out in GetLegalMoves.
//      Move(BoardLocation(13, 7), BoardLocation(12, 6), Piece(kRedPlayer, KING)),
//      Move(BoardLocation(13, 7), BoardLocation(12, 7), Piece(kRedPlayer, KING)),
//      Move(BoardLocation(13, 7), BoardLocation(12, 8), Piece(kRedPlayer, KING)),
//      Move(BoardLocation(13, 7), BoardLocation(13, 6), Piece(kRedPlayer, KING)),
//      Move(BoardLocation(13, 7), BoardLocation(13, 8), Piece(kRedPlayer, KING))));
//
//  // No castling rights
//  player_to_pieces = {
//    {kRedPlayer,
//      {
//        PlacedPiece(BoardLocation(13, 7), Piece(kRedPlayer, KING)),
//        PlacedPiece(BoardLocation(13, 3), Piece(kRedPlayer, ROOK)),
//        PlacedPiece(BoardLocation(13, 10), Piece(kRedPlayer, ROOK)),
//        PlacedPiece(BoardLocation(12, 6), Piece(kRedPlayer, PAWN)),
//        PlacedPiece(BoardLocation(12, 7), Piece(kRedPlayer, PAWN)),
//        PlacedPiece(BoardLocation(12, 8), Piece(kRedPlayer, PAWN)),
//      }},
//    {kBluePlayer,
//      {
//      }},
//    {kYellowPlayer,
//      {
//      }},
//    {kGreenPlayer,
//      {
//      }},
//  };
//
//  castling_rights = {
//    {kRedPlayer, CastlingRights(true, false)},
//    {kBluePlayer, CastlingRights()},
//    {kYellowPlayer, CastlingRights()},
//    {kGreenPlayer, CastlingRights()},
//  };
//
//  board = Board(kRedPlayer, player_to_pieces, std::nullopt, nullptr,
//      castling_rights);
//
//  moves = board.GetKingMoves(
//      PlacedPiece(BoardLocation(13, 7), Piece(kRedPlayer, KING)));
//
//  EXPECT_THAT(
//    moves,
//    UnorderedElementsAre(
//      Move(BoardLocation(13, 7), BoardLocation(13, 6), Piece(kRedPlayer, KING)),
//      Move(BoardLocation(13, 7), BoardLocation(13, 8), Piece(kRedPlayer, KING)),
//      Move(BoardLocation(13, 7), BoardLocation(13, 9), Piece(kRedPlayer, KING),
//           SimpleMove(BoardLocation(13, 10), BoardLocation(13, 8),
//                      Piece(kRedPlayer, ROOK)))));
//
//}

//TEST(BoardTest, GetLegalMoves_InCheck) {
//  // Moving a piece discovers check
//  // King moves into check
//
//  std::unordered_map<Player, std::vector<PlacedPiece>> player_to_pieces = {
//    {kRedPlayer,
//      {
//        PlacedPiece(BoardLocation(7, 7), Piece(kRedPlayer, KING)),
//        PlacedPiece(BoardLocation(6, 8), Piece(kRedPlayer, ROOK)),
//        PlacedPiece(BoardLocation(4, 6), Piece(kRedPlayer, BISHOP)),
//      }},
//    {kBluePlayer,
//      {
//        PlacedPiece(BoardLocation(5, 7), Piece(kBluePlayer, ROOK)),
//        PlacedPiece(BoardLocation(5, 5), Piece(kBluePlayer, KING)),
//      }},
//    {kYellowPlayer,
//      {
//      }},
//    {kGreenPlayer,
//      {
//      }},
//  };
//
//  Board board(kRedPlayer, player_to_pieces);
//  std::vector<Move> moves;
//
//  moves = board.GetLegalMoves();
//
//  EXPECT_THAT(
//      moves,
//      UnorderedElementsAre(
//        // Move away
//        Move(BoardLocation(7, 7), BoardLocation(7, 6), Piece(kRedPlayer, KING)),
//        Move(BoardLocation(7, 7), BoardLocation(7, 8), Piece(kRedPlayer, KING)),
//        Move(BoardLocation(7, 7), BoardLocation(8, 6), Piece(kRedPlayer, KING)),
//        Move(BoardLocation(7, 7), BoardLocation(8, 8), Piece(kRedPlayer, KING)),
//        // Block
//        Move(BoardLocation(6, 8), BoardLocation(6, 7), Piece(kRedPlayer, ROOK)),
//        // Capture
//        Move(BoardLocation(4, 6), BoardLocation(5, 7), Piece(kRedPlayer, BISHOP)),
//        // Capture king
//        Move(BoardLocation(4, 6), BoardLocation(5, 5), Piece(kRedPlayer, BISHOP))));
//
//}
//
//TEST(BoardTest, MakeMove) {
//  // Updates pieces:
//  // * Capture
//  // * Promotion
//  // * Unaffected
//
//  std::unordered_map<Player, std::vector<PlacedPiece>> player_to_pieces = {
//    {kRedPlayer,
//      {
//        PlacedPiece(BoardLocation(7, 7), Piece(kRedPlayer, KING)),
//        PlacedPiece(BoardLocation(6, 8), Piece(kRedPlayer, ROOK)),
//        PlacedPiece(BoardLocation(4, 6), Piece(kRedPlayer, BISHOP)),
//      }},
//    {kBluePlayer,
//      {
//        PlacedPiece(BoardLocation(5, 7), Piece(kBluePlayer, ROOK)),
//        PlacedPiece(BoardLocation(5, 5), Piece(kBluePlayer, KING)),
//      }},
//    {kYellowPlayer,
//      {
//      }},
//    {kGreenPlayer,
//      {
//      }},
//  };
//
//  Board board(kRedPlayer, player_to_pieces);
//
//  // Capture
//
//  Move move(BoardLocation(4, 6), BoardLocation(5, 7),
//            Piece(kRedPlayer, BISHOP));
//  Board updated = board.MakeMove(move);
//
//  player_to_pieces = {
//    {kRedPlayer,
//      {
//        PlacedPiece(BoardLocation(7, 7), Piece(kRedPlayer, KING)),
//        PlacedPiece(BoardLocation(6, 8), Piece(kRedPlayer, ROOK)),
//        PlacedPiece(BoardLocation(5, 7), Piece(kRedPlayer, BISHOP)),
//      }},
//    {kBluePlayer,
//      {
//        PlacedPiece(BoardLocation(5, 5), Piece(kBluePlayer, KING)),
//      }},
//    {kYellowPlayer,
//      {
//      }},
//    {kGreenPlayer,
//      {
//      }},
//  };
//
//  Board expected(kBluePlayer, player_to_pieces, move);
//  EXPECT_EQ(updated, expected);
//  EXPECT_EQ(updated.GetTurn(), kBluePlayer);
//
//  // Promotion
//
//  player_to_pieces = {
//    {kRedPlayer,
//      {
//        PlacedPiece(BoardLocation(7, 7), Piece(kRedPlayer, KING)),
//        PlacedPiece(BoardLocation(6, 8), Piece(kRedPlayer, ROOK)),
//        PlacedPiece(BoardLocation(4, 5), Piece(kRedPlayer, PAWN)),
//      }},
//    {kBluePlayer,
//      {
//        PlacedPiece(BoardLocation(5, 5), Piece(kBluePlayer, KING)),
//      }},
//    {kYellowPlayer,
//      {
//      }},
//    {kGreenPlayer,
//      {
//      }},
//  };
//  board = Board(kRedPlayer, player_to_pieces);
//
//  move = Move(BoardLocation(4, 5), BoardLocation(3, 5), Piece(kRedPlayer, PAWN),
//              QUEEN);
//  updated = board.MakeMove(move);
//
//  player_to_pieces = {
//    {kRedPlayer,
//      {
//        PlacedPiece(BoardLocation(7, 7), Piece(kRedPlayer, KING)),
//        PlacedPiece(BoardLocation(6, 8), Piece(kRedPlayer, ROOK)),
//        PlacedPiece(BoardLocation(3, 5), Piece(kRedPlayer, QUEEN)),
//      }},
//    {kBluePlayer,
//      {
//        PlacedPiece(BoardLocation(5, 5), Piece(kBluePlayer, KING)),
//      }},
//    {kYellowPlayer,
//      {
//      }},
//    {kGreenPlayer,
//      {
//      }},
//  };
//  expected = Board(kBluePlayer, player_to_pieces, move);
//  EXPECT_EQ(updated, expected);
//
//}
//
//TEST(BoardTest, Properties) {
//  // TeamToPlay
//  // GetTurn
//  // Equality
//
//  std::unordered_map<Player, std::vector<PlacedPiece>> player_to_pieces = {
//    {kRedPlayer,
//      {
//        PlacedPiece(BoardLocation(7, 7), Piece(kRedPlayer, KING)),
//        PlacedPiece(BoardLocation(6, 8), Piece(kRedPlayer, ROOK)),
//        PlacedPiece(BoardLocation(4, 6), Piece(kRedPlayer, BISHOP)),
//      }},
//    {kBluePlayer,
//      {
//        PlacedPiece(BoardLocation(5, 7), Piece(kBluePlayer, ROOK)),
//        PlacedPiece(BoardLocation(5, 5), Piece(kBluePlayer, KING)),
//      }},
//    {kYellowPlayer,
//      {
//      }},
//    {kGreenPlayer,
//      {
//      }},
//  };
//
//  Board board(kRedPlayer, player_to_pieces);
//  Board board2(kBluePlayer, player_to_pieces);
//
//  EXPECT_EQ(board.GetTurn(), kRedPlayer);
//  EXPECT_NE(board, board2);
//  EXPECT_EQ(board.TeamToPlay(), RED_YELLOW);
//  EXPECT_EQ(board2.TeamToPlay(), BLUE_GREEN);
//}
//
//TEST(BoardTest, GameOver) {
//  // Not game over
//  std::unordered_map<Player, std::vector<PlacedPiece>> player_to_pieces = {
//    {kRedPlayer,
//      {
//        PlacedPiece(BoardLocation(3, 3), Piece(kRedPlayer, KING)),
//      }},
//    {kBluePlayer,
//      {
//        PlacedPiece(BoardLocation(5, 5), Piece(kBluePlayer, KING)),
//      }},
//    {kYellowPlayer,
//      {
//        PlacedPiece(BoardLocation(7, 7), Piece(kYellowPlayer, KING)),
//      }},
//    {kGreenPlayer,
//      {
//        PlacedPiece(BoardLocation(9, 9), Piece(kGreenPlayer, KING)),
//      }},
//  };
//  Board board(kRedPlayer, player_to_pieces);
//  EXPECT_FALSE(board.IsGameOver());
//
//  // King capture
//  player_to_pieces = {
//    {kRedPlayer,
//      {
//        PlacedPiece(BoardLocation(3, 3), Piece(kRedPlayer, KING)),
//        PlacedPiece(BoardLocation(5, 5), Piece(kRedPlayer, BISHOP)),
//      }},
//    {kBluePlayer,
//      {
//      }},
//    {kYellowPlayer,
//      {
//        PlacedPiece(BoardLocation(7, 7), Piece(kYellowPlayer, KING)),
//      }},
//    {kGreenPlayer,
//      {
//        PlacedPiece(BoardLocation(9, 9), Piece(kGreenPlayer, KING)),
//      }},
//  };
//  Move move(BoardLocation(4, 4), BoardLocation(5, 5),
//            Piece(kRedPlayer, BISHOP));
//  Board board2(kBluePlayer, player_to_pieces, move, &board);
//  EXPECT_TRUE(board2.IsGameOver());
//  EXPECT_EQ(board2.WinningTeam(), RED_YELLOW);
//
//  // Checkmate
//  player_to_pieces = {
//    {kRedPlayer,
//      {
//        PlacedPiece(BoardLocation(3, 0), Piece(kRedPlayer, KING)),
//      }},
//    {kBluePlayer,
//      {
//        PlacedPiece(BoardLocation(5, 2), Piece(kBluePlayer, KING)),
//      }},
//    {kYellowPlayer,
//      {
//        PlacedPiece(BoardLocation(7, 7), Piece(kYellowPlayer, KING)),
//      }},
//    {kGreenPlayer,
//      {
//        PlacedPiece(BoardLocation(9, 9), Piece(kGreenPlayer, KING)),
//        PlacedPiece(BoardLocation(4, 1), Piece(kGreenPlayer, QUEEN)),
//      }},
//  };
//  move = Move(BoardLocation(4, 7), BoardLocation(4, 1),
//              Piece(kGreenPlayer, QUEEN));
//  board2 = Board(kRedPlayer, player_to_pieces, move, &board);
//  EXPECT_TRUE(board2.IsGameOver());
//  EXPECT_EQ(board2.WinningTeam(), BLUE_GREEN);
//
//  // Stalemate
//  player_to_pieces = {
//    {kRedPlayer,
//      {
//        PlacedPiece(BoardLocation(3, 0), Piece(kRedPlayer, KING)),
//      }},
//    {kBluePlayer,
//      {
//        PlacedPiece(BoardLocation(5, 2), Piece(kBluePlayer, KING)),
//      }},
//    {kYellowPlayer,
//      {
//        PlacedPiece(BoardLocation(7, 7), Piece(kYellowPlayer, KING)),
//      }},
//    {kGreenPlayer,
//      {
//        PlacedPiece(BoardLocation(9, 9), Piece(kGreenPlayer, KING)),
//        PlacedPiece(BoardLocation(4, 2), Piece(kGreenPlayer, QUEEN)),
//      }},
//  };
//  move = Move(BoardLocation(4, 7), BoardLocation(4, 2),
//              Piece(kGreenPlayer, QUEEN));
//  board2 = Board(kRedPlayer, player_to_pieces, move, &board);
//  EXPECT_TRUE(board2.IsGameOver());
//  EXPECT_EQ(board2.WinningTeam(), std::nullopt);
//}
//
//TEST(BoardTest, GetPieceStatistics) {
//  std::unordered_map<Player, std::vector<PlacedPiece>> player_to_pieces = {
//    {kRedPlayer,
//      {
//        PlacedPiece(BoardLocation(3, 3), Piece(kRedPlayer, KING)),
//        PlacedPiece(BoardLocation(4, 4), Piece(kRedPlayer, KNIGHT)),
//        PlacedPiece(BoardLocation(3, 4), Piece(kRedPlayer, KNIGHT)),
//        PlacedPiece(BoardLocation(3, 2), Piece(kRedPlayer, BISHOP)),
//      }},
//    {kBluePlayer,
//      {
//        PlacedPiece(BoardLocation(5, 5), Piece(kBluePlayer, KING)),
//        PlacedPiece(BoardLocation(8, 4), Piece(kBluePlayer, QUEEN)),
//        PlacedPiece(BoardLocation(8, 5), Piece(kBluePlayer, QUEEN)),
//      }},
//    {kYellowPlayer,
//      {
//        PlacedPiece(BoardLocation(7, 7), Piece(kYellowPlayer, KING)),
//        PlacedPiece(BoardLocation(3, 0), Piece(kYellowPlayer, ROOK)),
//      }},
//    {kGreenPlayer,
//      {
//        PlacedPiece(BoardLocation(9, 9), Piece(kGreenPlayer, KING)),
//        PlacedPiece(BoardLocation(3, 10), Piece(kGreenPlayer, PAWN)),
//        PlacedPiece(BoardLocation(4, 10), Piece(kGreenPlayer, PAWN)),
//      }},
//  };
//  Board board(kRedPlayer, player_to_pieces);
//  const auto& stats = board.GetPieceStatistics();
//
//  const auto& red_stats = stats.GetPlayerStatistics(kRedPlayer);
//  EXPECT_EQ(red_stats.GetNumPawns(), 0);
//  EXPECT_EQ(red_stats.GetNumKnights(), 2);
//  EXPECT_EQ(red_stats.GetNumBishops(), 1);
//  EXPECT_EQ(red_stats.GetNumRooks(), 0);
//  EXPECT_EQ(red_stats.GetNumQueens(), 0);
//
//  const auto& blue_stats = stats.GetPlayerStatistics(kBluePlayer);
//  EXPECT_EQ(blue_stats.GetNumPawns(), 0);
//  EXPECT_EQ(blue_stats.GetNumKnights(), 0);
//  EXPECT_EQ(blue_stats.GetNumBishops(), 0);
//  EXPECT_EQ(blue_stats.GetNumRooks(), 0);
//  EXPECT_EQ(blue_stats.GetNumQueens(), 2);
//
//  const auto& yellow_stats = stats.GetPlayerStatistics(kYellowPlayer);
//  EXPECT_EQ(yellow_stats.GetNumPawns(), 0);
//  EXPECT_EQ(yellow_stats.GetNumKnights(), 0);
//  EXPECT_EQ(yellow_stats.GetNumBishops(), 0);
//  EXPECT_EQ(yellow_stats.GetNumRooks(), 1);
//  EXPECT_EQ(yellow_stats.GetNumQueens(), 0);
//
//  const auto& green_stats = stats.GetPlayerStatistics(kGreenPlayer);
//  EXPECT_EQ(green_stats.GetNumPawns(), 2);
//  EXPECT_EQ(green_stats.GetNumKnights(), 0);
//  EXPECT_EQ(green_stats.GetNumBishops(), 0);
//  EXPECT_EQ(green_stats.GetNumRooks(), 0);
//  EXPECT_EQ(green_stats.GetNumQueens(), 0);
//}
//
//TEST(BoardTest, KingLocation) {
//  // Each player
//  // King captured
//  std::unordered_map<Player, std::vector<PlacedPiece>> player_to_pieces = {
//    {kRedPlayer,
//      {
//        PlacedPiece(BoardLocation(3, 3), Piece(kRedPlayer, KING)),
//        PlacedPiece(BoardLocation(4, 4), Piece(kRedPlayer, KNIGHT)),
//        PlacedPiece(BoardLocation(3, 4), Piece(kRedPlayer, KNIGHT)),
//        PlacedPiece(BoardLocation(3, 2), Piece(kRedPlayer, BISHOP)),
//      }},
//    {kBluePlayer,
//      {
//        PlacedPiece(BoardLocation(5, 5), Piece(kBluePlayer, KING)),
//        PlacedPiece(BoardLocation(8, 4), Piece(kBluePlayer, QUEEN)),
//        PlacedPiece(BoardLocation(8, 5), Piece(kBluePlayer, QUEEN)),
//      }},
//    {kYellowPlayer,
//      {
//        PlacedPiece(BoardLocation(3, 0), Piece(kYellowPlayer, ROOK)),
//      }},
//    {kGreenPlayer,
//      {
//        PlacedPiece(BoardLocation(9, 9), Piece(kGreenPlayer, KING)),
//        PlacedPiece(BoardLocation(3, 10), Piece(kGreenPlayer, PAWN)),
//        PlacedPiece(BoardLocation(4, 10), Piece(kGreenPlayer, PAWN)),
//      }},
//  };
//  Board board(kRedPlayer, player_to_pieces);
//
//  EXPECT_EQ(board.KingLocation(kRedPlayer), BoardLocation(3, 3));
//  EXPECT_EQ(board.KingLocation(kBluePlayer), BoardLocation(5, 5));
//  EXPECT_EQ(board.KingLocation(kYellowPlayer), std::nullopt);
//  EXPECT_EQ(board.KingLocation(kGreenPlayer), BoardLocation(9, 9));
//}

//TEST(BoardTest, CreateStandardSetup) {
//  // All players, pieces
//
//  Board board = Board::CreateStandardSetup();
//
//  EXPECT_EQ(board.GetPiece(BoardLocation(13, 3)).value(), Piece(kRedPlayer, ROOK));
//  EXPECT_EQ(board.GetPiece(BoardLocation(13, 4)).value(), Piece(kRedPlayer, KNIGHT));
//  EXPECT_EQ(board.GetPiece(BoardLocation(13, 5)).value(), Piece(kRedPlayer, BISHOP));
//  EXPECT_EQ(board.GetPiece(BoardLocation(13, 6)).value(), Piece(kRedPlayer, QUEEN));
//  EXPECT_EQ(board.GetPiece(BoardLocation(13, 7)).value(), Piece(kRedPlayer, KING));
//  EXPECT_EQ(board.GetPiece(BoardLocation(13, 8)).value(), Piece(kRedPlayer, BISHOP));
//  EXPECT_EQ(board.GetPiece(BoardLocation(13, 9)).value(), Piece(kRedPlayer, KNIGHT));
//  EXPECT_EQ(board.GetPiece(BoardLocation(13, 10)).value(), Piece(kRedPlayer, ROOK));
//
//  EXPECT_EQ(board.GetPiece(BoardLocation(12, 3)).value(), Piece(kRedPlayer, PAWN));
//  EXPECT_EQ(board.GetPiece(BoardLocation(12, 4)).value(), Piece(kRedPlayer, PAWN));
//  EXPECT_EQ(board.GetPiece(BoardLocation(12, 5)).value(), Piece(kRedPlayer, PAWN));
//  EXPECT_EQ(board.GetPiece(BoardLocation(12, 6)).value(), Piece(kRedPlayer, PAWN));
//  EXPECT_EQ(board.GetPiece(BoardLocation(12, 7)).value(), Piece(kRedPlayer, PAWN));
//  EXPECT_EQ(board.GetPiece(BoardLocation(12, 8)).value(), Piece(kRedPlayer, PAWN));
//  EXPECT_EQ(board.GetPiece(BoardLocation(12, 9)).value(), Piece(kRedPlayer, PAWN));
//  EXPECT_EQ(board.GetPiece(BoardLocation(12, 10)).value(), Piece(kRedPlayer, PAWN));
//
//  EXPECT_EQ(board.GetPiece(BoardLocation(3, 0)).value(), Piece(kBluePlayer, ROOK));
//  EXPECT_EQ(board.GetPiece(BoardLocation(4, 0)).value(), Piece(kBluePlayer, KNIGHT));
//  EXPECT_EQ(board.GetPiece(BoardLocation(5, 0)).value(), Piece(kBluePlayer, BISHOP));
//  EXPECT_EQ(board.GetPiece(BoardLocation(6, 0)).value(), Piece(kBluePlayer, QUEEN));
//  EXPECT_EQ(board.GetPiece(BoardLocation(7, 0)).value(), Piece(kBluePlayer, KING));
//  EXPECT_EQ(board.GetPiece(BoardLocation(8, 0)).value(), Piece(kBluePlayer, BISHOP));
//  EXPECT_EQ(board.GetPiece(BoardLocation(9, 0)).value(), Piece(kBluePlayer, KNIGHT));
//  EXPECT_EQ(board.GetPiece(BoardLocation(10, 0)).value(), Piece(kBluePlayer, ROOK));
//
//  EXPECT_EQ(board.GetPiece(BoardLocation(3, 1)).value(), Piece(kBluePlayer, PAWN));
//  EXPECT_EQ(board.GetPiece(BoardLocation(4, 1)).value(), Piece(kBluePlayer, PAWN));
//  EXPECT_EQ(board.GetPiece(BoardLocation(5, 1)).value(), Piece(kBluePlayer, PAWN));
//  EXPECT_EQ(board.GetPiece(BoardLocation(6, 1)).value(), Piece(kBluePlayer, PAWN));
//  EXPECT_EQ(board.GetPiece(BoardLocation(7, 1)).value(), Piece(kBluePlayer, PAWN));
//  EXPECT_EQ(board.GetPiece(BoardLocation(8, 1)).value(), Piece(kBluePlayer, PAWN));
//  EXPECT_EQ(board.GetPiece(BoardLocation(9, 1)).value(), Piece(kBluePlayer, PAWN));
//  EXPECT_EQ(board.GetPiece(BoardLocation(10, 1)).value(), Piece(kBluePlayer, PAWN));
//
//  EXPECT_EQ(board.GetPiece(BoardLocation(0, 3)).value(), Piece(kYellowPlayer, ROOK));
//  EXPECT_EQ(board.GetPiece(BoardLocation(0, 4)).value(), Piece(kYellowPlayer, KNIGHT));
//  EXPECT_EQ(board.GetPiece(BoardLocation(0, 5)).value(), Piece(kYellowPlayer, BISHOP));
//  EXPECT_EQ(board.GetPiece(BoardLocation(0, 6)).value(), Piece(kYellowPlayer, KING));
//  EXPECT_EQ(board.GetPiece(BoardLocation(0, 7)).value(), Piece(kYellowPlayer, QUEEN));
//  EXPECT_EQ(board.GetPiece(BoardLocation(0, 8)).value(), Piece(kYellowPlayer, BISHOP));
//  EXPECT_EQ(board.GetPiece(BoardLocation(0, 9)).value(), Piece(kYellowPlayer, KNIGHT));
//  EXPECT_EQ(board.GetPiece(BoardLocation(0, 10)).value(), Piece(kYellowPlayer, ROOK));
//
//  EXPECT_EQ(board.GetPiece(BoardLocation(1, 3)).value(), Piece(kYellowPlayer, PAWN));
//  EXPECT_EQ(board.GetPiece(BoardLocation(1, 4)).value(), Piece(kYellowPlayer, PAWN));
//  EXPECT_EQ(board.GetPiece(BoardLocation(1, 5)).value(), Piece(kYellowPlayer, PAWN));
//  EXPECT_EQ(board.GetPiece(BoardLocation(1, 6)).value(), Piece(kYellowPlayer, PAWN));
//  EXPECT_EQ(board.GetPiece(BoardLocation(1, 7)).value(), Piece(kYellowPlayer, PAWN));
//  EXPECT_EQ(board.GetPiece(BoardLocation(1, 8)).value(), Piece(kYellowPlayer, PAWN));
//  EXPECT_EQ(board.GetPiece(BoardLocation(1, 9)).value(), Piece(kYellowPlayer, PAWN));
//  EXPECT_EQ(board.GetPiece(BoardLocation(1, 10)).value(), Piece(kYellowPlayer, PAWN));
//
//  EXPECT_EQ(board.GetPiece(BoardLocation(3, 13)).value(), Piece(kGreenPlayer, ROOK));
//  EXPECT_EQ(board.GetPiece(BoardLocation(4, 13)).value(), Piece(kGreenPlayer, KNIGHT));
//  EXPECT_EQ(board.GetPiece(BoardLocation(5, 13)).value(), Piece(kGreenPlayer, BISHOP));
//  EXPECT_EQ(board.GetPiece(BoardLocation(6, 13)).value(), Piece(kGreenPlayer, KING));
//  EXPECT_EQ(board.GetPiece(BoardLocation(7, 13)).value(), Piece(kGreenPlayer, QUEEN));
//  EXPECT_EQ(board.GetPiece(BoardLocation(8, 13)).value(), Piece(kGreenPlayer, BISHOP));
//  EXPECT_EQ(board.GetPiece(BoardLocation(9, 13)).value(), Piece(kGreenPlayer, KNIGHT));
//  EXPECT_EQ(board.GetPiece(BoardLocation(10, 13)).value(), Piece(kGreenPlayer, ROOK));
//
//  EXPECT_EQ(board.GetPiece(BoardLocation(3, 12)).value(), Piece(kGreenPlayer, PAWN));
//  EXPECT_EQ(board.GetPiece(BoardLocation(4, 12)).value(), Piece(kGreenPlayer, PAWN));
//  EXPECT_EQ(board.GetPiece(BoardLocation(5, 12)).value(), Piece(kGreenPlayer, PAWN));
//  EXPECT_EQ(board.GetPiece(BoardLocation(6, 12)).value(), Piece(kGreenPlayer, PAWN));
//  EXPECT_EQ(board.GetPiece(BoardLocation(7, 12)).value(), Piece(kGreenPlayer, PAWN));
//  EXPECT_EQ(board.GetPiece(BoardLocation(8, 12)).value(), Piece(kGreenPlayer, PAWN));
//  EXPECT_EQ(board.GetPiece(BoardLocation(9, 12)).value(), Piece(kGreenPlayer, PAWN));
//  EXPECT_EQ(board.GetPiece(BoardLocation(10, 12)).value(), Piece(kGreenPlayer, PAWN));
//
//}

//TEST(BoardTest, WholeGame) {
//  Board board = Board::CreateStandardSetup();
//  while (true) {
//    std::cout << board << std::endl;
//    auto res = board.GetLegalMoves();
//    if (std::get<0>(res) != IN_PROGRESS) {
//      break;
//    }
//    const auto& moves = std::get<1>(res);
//    assert(moves.size() > 0);
//    const auto& move = moves[0];
//    board.MakeMove(move);
//  }
//
//}

//TEST(BoardTest, StandardMoves) {
//  Board board = Board::CreateStandardSetup();
//
//  board.MakeMove(Move(BoardLocation(12, 7), BoardLocation(10, 7)));
//  board.MakeMove(Move(BoardLocation(7, 1), BoardLocation(7, 3)));
//  board.MakeMove(Move(BoardLocation(1, 6), BoardLocation(3, 6)));
//  board.MakeMove(Move(BoardLocation(4, 13), BoardLocation(5, 11)));
//
//  std::cout << board;
//
//  std::cout << "piece evaluation: " << board.PieceEvaluation() << std::endl;
//  std::cout << "mobility evaluation: " << board.MobilityEvaluation() << std::endl;
//
//  auto res = board.GetLegalMoves();
//  ASSERT_EQ(std::get<0>(res), IN_PROGRESS);
//  for (const auto& move : std::get<1>(res)) {
//    std::cout << "red move: " << move << std::endl;
//  }
//}

TEST(HelperFunctionsTest, OtherTeam) {
  EXPECT_EQ(OtherTeam(RED_YELLOW), BLUE_GREEN);
  EXPECT_EQ(OtherTeam(BLUE_GREEN), RED_YELLOW);
}

//TEST(HelperFunctionsTest, GetTeam) {
//  EXPECT_EQ(GetTeam(RED), RED_YELLOW);
//  EXPECT_EQ(GetTeam(YELLOW), RED_YELLOW);
//  EXPECT_EQ(GetTeam(BLUE), BLUE_GREEN);
//  EXPECT_EQ(GetTeam(GREEN), BLUE_GREEN);
//}

//TEST(HelperFunctionsTest, GetNextPlayer) {
//  EXPECT_EQ(GetNextPlayer(kRedPlayer), kBluePlayer);
//  EXPECT_EQ(GetNextPlayer(kBluePlayer), kYellowPlayer);
//  EXPECT_EQ(GetNextPlayer(kYellowPlayer), kGreenPlayer);
//  EXPECT_EQ(GetNextPlayer(kGreenPlayer), kRedPlayer);
//}
//
//TEST(HelperFunctionsTest, GetPreviousPlayer) {
//  EXPECT_EQ(GetPreviousPlayer(kRedPlayer), kGreenPlayer);
//  EXPECT_EQ(GetPreviousPlayer(kBluePlayer), kRedPlayer);
//  EXPECT_EQ(GetPreviousPlayer(kYellowPlayer), kBluePlayer);
//  EXPECT_EQ(GetPreviousPlayer(kGreenPlayer), kYellowPlayer);
//}

TEST(BoardTest, KeyTest) {
  auto board = Board::CreateStandardSetup();
  int64_t h0 = board->HashKey();
  board->MakeMove(Move(BoardLocation(12, 7), BoardLocation(11, 7))); // h3
  int64_t h1 = board->HashKey();
  board->MakeMove(Move(BoardLocation(7, 1), BoardLocation(7, 2))); // c7
  int64_t h2 = board->HashKey();
  board->MakeMove(Move(BoardLocation(1, 6), BoardLocation(2, 6))); // g12
  int64_t h3 = board->HashKey();
  board->MakeMove(Move(BoardLocation(6, 12), BoardLocation(6, 11))); // l8
  int64_t h4 = board->HashKey();
  board->MakeMove(Move(BoardLocation(13, 6), BoardLocation(7, 12),
                  board->GetPiece(BoardLocation(7, 12)))); // Qxm7
  int64_t h5 = board->HashKey();
  board->MakeMove(Move(BoardLocation(6, 0), BoardLocation(12, 6),
                  board->GetPiece(BoardLocation(12, 6)))); // Qxg2

  board->UndoMove();
  int64_t h5_1 = board->HashKey();
  board->UndoMove();
  int64_t h4_1 = board->HashKey();
  board->UndoMove();
  int64_t h3_1 = board->HashKey();
  board->UndoMove();
  int64_t h2_1 = board->HashKey();
  board->UndoMove();
  int64_t h1_1 = board->HashKey();
  board->UndoMove();
  int64_t h0_1 = board->HashKey();

  EXPECT_EQ(h0, h0_1);
  EXPECT_EQ(h1, h1_1);
  EXPECT_EQ(h2, h2_1);
  EXPECT_EQ(h3, h3_1);
  EXPECT_EQ(h4, h4_1);
  EXPECT_EQ(h5, h5_1);

  EXPECT_NE(h0, h1);
  EXPECT_NE(h0, h2);
  EXPECT_NE(h0, h3);
  EXPECT_NE(h0, h5);
}

TEST(BoardTest, KeyTest_NullMove) {
  auto board = Board::CreateStandardSetup();
  int64_t h0 = board->HashKey();
  board->MakeNullMove();
  int64_t h1 = board->HashKey();
  board->MakeNullMove();

  board->UndoNullMove();
  int64_t h1_1 = board->HashKey();
  board->UndoNullMove();
  int64_t h0_1 = board->HashKey();

  EXPECT_EQ(h0, h0_1);
  EXPECT_EQ(h1, h1_1);
  EXPECT_NE(h0, h1);
}

TEST(BoardTest, IsKingInCheck) {
  auto board = Board::CreateStandardSetup();
  board->MakeMove(Move(BoardLocation(12, 7), BoardLocation(11, 7))); // h3
  board->MakeMove(Move(BoardLocation(7, 1), BoardLocation(7, 2))); // c7
  board->MakeMove(Move(BoardLocation(1, 6), BoardLocation(2, 6))); // g12
  board->MakeMove(Move(BoardLocation(6, 12), BoardLocation(6, 11))); // l8
  board->MakeMove(Move(BoardLocation(13, 6), BoardLocation(7, 12))); // Qxm7

  EXPECT_TRUE(board->IsKingInCheck(Player(GREEN)));

  board->UndoMove();

  board->MakeMove(Move(BoardLocation(13, 6), BoardLocation(10, 9))); // Qj4
  board->MakeMove(Move(BoardLocation(6, 0), BoardLocation(9, 3))); // Qd5
  board->MakeMove(Move(BoardLocation(7, 0), BoardLocation(6, 1))); // Qxb8
  EXPECT_TRUE(board->IsKingInCheck(Player(BLUE)));
  board->MakeMove(Move(BoardLocation(7, 13), BoardLocation(1, 7))); // Qxh13
  EXPECT_TRUE(board->IsKingInCheck(Player(YELLOW)));
}

TEST(BoardTest, IsKingInCheckFalse) {
  auto board = Board::CreateStandardSetup();

  EXPECT_FALSE(board->IsKingInCheck(Player(RED)));
  EXPECT_FALSE(board->IsKingInCheck(Player(BLUE)));
  EXPECT_FALSE(board->IsKingInCheck(Player(YELLOW)));
  EXPECT_FALSE(board->IsKingInCheck(Player(GREEN)));

  board->MakeMove(Move(BoardLocation(12, 7), BoardLocation(11, 7))); // h3

  EXPECT_FALSE(board->IsKingInCheck(Player(RED)));
  EXPECT_FALSE(board->IsKingInCheck(Player(BLUE)));
  EXPECT_FALSE(board->IsKingInCheck(Player(YELLOW)));
  EXPECT_FALSE(board->IsKingInCheck(Player(GREEN)));
}

//TEST(BoardTest, Enpassant) {
//  auto board = 
//}

namespace {

std::optional<Move> FindMove(
    Board& board, const BoardLocation& from, const BoardLocation& to) {
  Move moves[300];
  size_t num_moves = board.GetPseudoLegalMoves2(moves, 300);
  for (size_t i = 0; i < num_moves; i++) {
    const auto& move = moves[i];
    if (move.From() == from
        && move.To() == to) {
      return move;
    }
  }
  return std::nullopt;
}

}  // namespace

TEST(BoardTest, MakeAndUndoMoves) {
  auto board = ParseBoardFromFEN("R-0,0,0,0-1,1,1,1-1,0,1,1-0,0,0,0-2-x,x,x,yR,yN,1,yK,1,yB,yN,yR,x,x,x/x,x,x,yP,yP,yP,1,yP,yP,yP,yP,x,x,x/x,x,x,3,yP,4,x,x,x/bR,bP,10,gP,gR/bN,bP,10,gP,gN/bB,2,bP,8,gP,1/bQ,bP,9,gP,1,gK/bK,bP,bP,1,yQ,7,gP,1/bB,11,gP,gB/bN,1,bP,6,gB,2,gP,gN/3,bR,1,rP,6,gP,gR/x,x,x,4,rP,3,x,x,x/x,x,x,rP,rP,1,rP,1,rP,rP,rP,x,x,x/x,x,x,rR,1,rB,rQ,rK,1,rN,rR,x,x,x");

  int64_t hash = board->HashKey();

  board->MakeMove(*FindMove(*board, Loc(12, 3), Loc(11, 3)));
  board->MakeMove(*FindMove(*board, Loc(6, 0), Loc(5, 1)));
  board->MakeMove(*FindMove(*board, Loc(0, 4), Loc(2, 5)));
  board->MakeNullMove();
  board->MakeMove(*FindMove(*board, Loc(13, 6), Loc(10, 3)));
  board->MakeMove(*FindMove(*board, Loc(9, 2), Loc(10, 3)));
  board->MakeMove(*FindMove(*board, Loc(7, 4), Loc(9, 2)));

  board->UndoMove();
  board->UndoMove();
  board->UndoMove();
  board->UndoNullMove();
  board->UndoMove();
  board->UndoMove();
  board->UndoMove();

//  board->MakeMove(FindMove(*board, Loc(13, 6), Loc(10, 3)).value());
//  board->MakeMove(FindMove(*board, Loc(6, 0), Loc(1, 5)).value());
//  board->MakeMove(FindMove(*board, Loc(0, 6), Loc(1, 5)).value());
//  board->MakeMove(FindMove(*board, Loc(9, 9), Loc(11, 7)).value());
//  board->MakeMove(FindMove(*board, Loc(12, 3), Loc(11, 3)).value());
//  board->MakeMove(FindMove(*board, Loc(9, 2), Loc(10, 3)).value());
//  board->MakeMove(FindMove(*board, Loc(7, 4), Loc(9, 2)).value());
//
//  while (board->NumMoves() > 0) {
//    board->UndoMove();
//  }

  EXPECT_EQ(hash, board->HashKey());
}

namespace {

Move MakeMove(const Board& board, BoardLocation from, BoardLocation to) {
  return Move(from, to, board.GetPiece(to));
}

}  // namespace

TEST(BoardTest, DeliversCheck) {
  auto board = ParseBoardFromFEN("R-0,0,0,0-1,1,1,1-1,1,1,1-0,0,0,0-0-x,x,x,yR,yN,yB,yK,yQ,yB,yN,yR,x,x,x/x,x,x,yP,yP,yP,1,yP,yP,yP,yP,x,x,x/x,x,x,3,yP,4,x,x,x/bR,bP,10,gP,gR/bN,bP,10,gP,gN/bB,bP,10,gP,gB/bQ,bP,9,gP,1,gK/bK,1,bP,9,gP,gQ/bB,bP,10,gP,gB/bN,bP,10,gP,gN/bR,bP,10,gP,gR/x,x,x,4,rP,3,x,x,x/x,x,x,rP,rP,rP,rP,1,rP,rP,rP,x,x,x/x,x,x,rR,rN,rB,rQ,rK,rB,rN,rR,x,x,x");

  Move move;

  move = MakeMove(*board, BoardLocation(13, 6), BoardLocation(7, 12));
  EXPECT_TRUE(board->DeliversCheck(move));
  EXPECT_TRUE(move.DeliversCheck(*board));
  EXPECT_TRUE(move.DeliversCheck(*board));

  move = MakeMove(*board, BoardLocation(13, 8), BoardLocation(7, 2));
  EXPECT_FALSE(board->DeliversCheck(move));
  EXPECT_FALSE(move.DeliversCheck(*board));
  EXPECT_FALSE(move.DeliversCheck(*board));

  move = MakeMove(*board, BoardLocation(13, 4), BoardLocation(11, 3));
  EXPECT_FALSE(board->DeliversCheck(move));
  EXPECT_FALSE(move.DeliversCheck(*board));
  EXPECT_FALSE(move.DeliversCheck(*board));

  move = MakeMove(*board, BoardLocation(13, 7), BoardLocation(12, 7));
  EXPECT_FALSE(board->DeliversCheck(move));
  EXPECT_FALSE(move.DeliversCheck(*board));
  EXPECT_FALSE(move.DeliversCheck(*board));

  move = MakeMove(*board, BoardLocation(12, 5), BoardLocation(11, 5));
  EXPECT_FALSE(board->DeliversCheck(move));
  EXPECT_FALSE(move.DeliversCheck(*board));
  EXPECT_FALSE(move.DeliversCheck(*board));

  board = ParseBoardFromFEN("R-0,0,0,0-1,1,1,1-1,1,1,1-0,0,0,0-0-x,x,x,yR,yN,yB,yK,yQ,yB,yN,yR,x,x,x/x,x,x,yP,yP,yP,1,yP,yP,yP,yP,x,x,x/x,x,x,3,yP,4,x,x,x/bR,bP,10,gP,gR/bN,bP,10,gP,gN/bB,bP,10,gP,gB/bQ,bP,9,gP,1,gK/bK,1,bP,9,gP,gQ/bB,bP,10,gP,gB/bN,bP,10,gP,gN/bR,bP,10,gP,gR/x,x,x,1,rB,2,rP,3,x,x,x/x,x,x,rP,rP,rP,rP,1,rP,rP,rP,x,x,x/x,x,x,rR,rN,rB,rQ,rK,1,rN,rR,x,x,x");

  move = MakeMove(*board, BoardLocation(11, 4), BoardLocation(8, 1));
  EXPECT_TRUE(board->DeliversCheck(move));
  EXPECT_TRUE(move.DeliversCheck(*board));
  EXPECT_TRUE(move.DeliversCheck(*board));

  board = ParseBoardFromFEN("R-0,0,0,0-1,1,1,1-1,1,1,1-0,0,0,0-0-x,x,x,yR,yN,yB,yK,yQ,yB,yN,yR,x,x,x/x,x,x,yP,yP,yP,1,yP,yP,yP,yP,x,x,x/x,x,x,3,yP,4,x,x,x/bR,bP,10,gP,gR/bN,bP,10,gP,gN/bB,bP,2,rN,7,gP,gB/bQ,bP,9,gP,1,gK/bK,1,bP,9,gP,gQ/bB,bP,10,gP,gB/bN,bP,10,gP,gN/bR,bP,10,gP,gR/x,x,x,1,rB,2,rP,3,x,x,x/x,x,x,rP,rP,rP,rP,1,rP,rP,rP,x,x,x/x,x,x,rR,1,rB,rQ,rK,1,rN,rR,x,x,x");

  move = MakeMove(*board, BoardLocation(5, 4), BoardLocation(6, 2));
  EXPECT_TRUE(board->DeliversCheck(move));
  EXPECT_TRUE(move.DeliversCheck(*board));
  EXPECT_TRUE(move.DeliversCheck(*board));

  board = ParseBoardFromFEN("R-0,0,0,0-1,1,1,1-1,1,1,1-0,0,0,0-0-x,x,x,yR,yN,yB,yK,yQ,yB,yN,yR,x,x,x/x,x,x,yP,yP,yP,1,yP,yP,yP,yP,x,x,x/x,x,x,3,yP,4,x,x,x/bR,bP,10,gP,gR/bN,bP,10,gP,gN/bB,bP,2,rN,7,gP,gB/bQ,bP,9,gP,1,gK/bK,1,bP,4,rR,4,gP,gQ/bB,bP,10,gP,gB/bN,bP,10,gP,gN/bR,bP,10,gP,gR/x,x,x,1,rB,2,rP,3,x,x,x/x,x,x,rP,rP,rP,rP,1,rP,rP,rP,x,x,x/x,x,x,2,rB,rQ,rK,1,rN,rR,x,x,x");

  move = MakeMove(*board, BoardLocation(7, 7), BoardLocation(7, 2));
  EXPECT_TRUE(board->DeliversCheck(move));
  EXPECT_TRUE(move.DeliversCheck(*board));
  EXPECT_TRUE(move.DeliversCheck(*board));

  move = MakeMove(*board, BoardLocation(7, 7), BoardLocation(7, 12));
  EXPECT_FALSE(board->DeliversCheck(move));
  EXPECT_FALSE(move.DeliversCheck(*board));
  EXPECT_FALSE(move.DeliversCheck(*board));

  board = ParseBoardFromFEN("Y-0,0,0,0-1,1,1,1-1,1,1,1-0,0,0,0-0-x,x,x,yR,yN,1,yK,1,yB,yN,yR,x,x,x/x,x,x,yP,yP,yP,1,yQ,yP,bQ,yP,x,x,x/x,x,x,3,yP,4,x,x,x/bR,bP,10,gP,gR/bN,bP,10,gP,gN/bB,bP,2,rQ,7,gP,gB/1,bP,9,yB,2/bK,1,bP,9,gP,gK/bB,bP,10,gP,gB/bN,bP,10,gP,gN/bR,bP,10,gP,gR/x,x,x,4,rP,3,x,x,x/x,x,x,rP,rP,rP,rP,1,rP,rP,rP,x,x,x/x,x,x,rR,rN,rB,1,rK,rB,rN,rR,x,x,x");

  move = MakeMove(*board, BoardLocation(1, 7), BoardLocation(7, 13));

  EXPECT_TRUE(board->DeliversCheck(move));
  EXPECT_TRUE(move.DeliversCheck(*board));
  EXPECT_TRUE(move.DeliversCheck(*board));

}


}  // namespace chess


