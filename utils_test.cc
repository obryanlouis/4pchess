#include "gmock/gmock.h"
#include <chrono>
#include <gtest/gtest.h>
#include <unordered_map>
#include <vector>

#include "board.h"
#include "utils.h"

namespace chess {

using ::testing::UnorderedElementsAre;


TEST(UtilsTest, ParseFENTest) {
  auto board = ParseBoardFromFEN("R-0,0,0,0-1,1,1,1-1,1,1,0-0,0,0,0-0-x,x,x,yR,yN,yB,1,yK,yB,1,yR,x,x,x/x,x,x,1,yP,1,yP,1,yP,yP,1,x,x,x/x,x,x,2,yP,1,yP,yN,2,x,x,x/bR,bP,1,yP,6,yP,1,gP,1/1,bP,11,gR/bB,10,gP,gP,gB/1,bP,2,bN,7,gP,1/bK,1,bP,8,gP,1,gK/bB,bP,10,gP,gB/bN,bP,bQ,7,rP,1,gP,gN/bR,bP,4,rQ,6,gR/x,x,x,2,rP,2,gP,2,x,x,x/x,x,x,rP,rP,1,rP,rP,rP,1,rP,x,x,x/x,x,x,rR,rN,rB,1,rK,rB,1,rR,x,x,x");
  EXPECT_NE(board, nullptr);

  std::cout << *board << std::endl;
}


}  // namespace chess


