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
  Board board = Board::CreateStandardSetup();
  PlayerOptions options;
  options.enable_transposition_table = true;
  AlphaBetaPlayer player(options);

  std::chrono::milliseconds time_limit(10000);
  auto res = player.MakeMove(board, time_limit);

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

//TEST(Speed, Iteration) {
//  auto start = std::chrono::system_clock::now();
//
////  Board board = Board::CreateStandardSetup();
////  AlphaBetaPlayer player;
////
////  std::chrono::milliseconds time_limit(1000);
////  auto move = player.MakeMove(board, time_limit);
////
////  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
////      std::chrono::system_clock::now() - start);
////  std::cout << "Duration (ms): " << duration.count() << std::endl;
////
////  if (move.has_value()) {
////    std::cout << "move: " << move.value() << std::endl;
////  } else {
////    std::cout << "move missing" << std::endl;
////  }
//
//
////  Board board = Board::CreateStandardSetup();
////  int counter = 0;
////  for (int i = 0; i < 1'000'000; i++) {
////    auto res = board.GetLegalMoves();
////    counter = std::get<1>(res).size();
////  }
//
//  int counter = 0;
//  Piece piece(kRedPlayer, PAWN);
//  BoardLocation loc_1(3, 0);
//  BoardLocation loc_2(4, 10);
//
//  struct MyMove {
//    const BoardLocation* loc_1;
//    const BoardLocation* loc_2;
//    const Piece* piece;
//
//    MyMove(const BoardLocation* a,
//           const BoardLocation* b,
//           const Piece* p)
//      : loc_1(a), loc_2(b), piece(p) { }
//  };
//
//  std::vector<MyMove> moves;
//  moves.reserve(20);
//    for (int i = 0; i < 20; i++) {
//      moves.emplace_back(&loc_1, &loc_2, &piece);
//    }
//
//  for (int i = 0; i < 50'000'000; i++) {
//
////    std::vector<Move> moves;
////    moves.reserve(20);
////    for (int i = 0; i < 20; i++) {
////      moves.emplace_back(BoardLocation(3, 0), BoardLocation(4, 10),
////          Piece(kRedPlayer, PAWN));
////    }
//
//    for (int i = 0; i < 20; i++) {
////      moves.emplace_back(&loc_1, &loc_2, &piece);
//      moves[i] = MyMove(&loc_1, &loc_2, &piece);
////      auto& move = moves[i];
////      move.loc_1 = &loc_1;
////      move.loc_2 = &loc_2;
////      move.piece = &piece;
//      for (int j = 0; j < 2000; j++) {
//        counter += j;
//      }
//    }
////    moves.clear();
//
//    counter += moves.size();
//  }
//
//
//  auto end = std::chrono::system_clock::now();
//
//  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
//      end - start);
//  std::cout << "Duration (ms): " << duration.count() << std::endl;
//
//  std::cout << "count: " << counter << std::endl;
//
//}

}  // namespace
}  // namespace chess

