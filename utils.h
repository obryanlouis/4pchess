#ifndef _UTILS_H_
#define _UTILS_H_

#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "board.h"


namespace chess {

std::vector<std::string> SplitStrOnWhitespace(const std::string& x);

std::vector<std::string> SplitStr(std::string s, std::string delimiter);

std::optional<int> ParseInt(const std::string& input);

std::optional<std::vector<bool>> ParseCastlingAvailability(
    const std::string& fen_substr);

std::shared_ptr<Board> ParseBoardFromFEN(const std::string& fen);

void SendInfoMessage(const std::string& message);

void SendInvalidCommandMessage(const std::string& line);

std::optional<Move> ParseMove(Board& board, const std::string& move_str);

}  // namespace chess

#endif  // _UTILS_H_
