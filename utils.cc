#include "utils.h"

#include <tuple>
#include <cctype>
#include <iostream>
#include <exception>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "board.h"


namespace chess {

std::vector<std::string> SplitStrOnWhitespace(const std::string& x) {
  std::stringstream ss(x);
  std::string part;
  std::vector<std::string> parts;
  while (ss >> part) {
    parts.push_back(part);
  }
  return parts;
}

std::vector<std::string> SplitStr(std::string s, std::string delimiter) {
  size_t pos_start = 0;
  size_t pos_end;
  size_t delim_len = delimiter.length();
  std::string token;
  std::vector<std::string> res;

  while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos) {
    token = s.substr(pos_start, pos_end - pos_start);
    pos_start = pos_end + delim_len;
    res.push_back(token);
  }

  res.push_back(s.substr(pos_start));
  return res;
}

std::optional<int> ParseInt(const std::string& input) {
  try {
    return std::stoi(input);
  } catch(std::exception const& ex) {
    return std::nullopt;
  }
}

std::optional<std::vector<bool>> ParseCastlingAvailability(
    const std::string& fen_substr) {
  std::vector<std::string> parts = SplitStr(fen_substr, ",");
  if (parts.size() != 4) {
    return std::nullopt;
  }
  std::vector<bool> availability;
  availability.reserve(4);
  for (const auto& part : parts) {
    if (part == "0") {
      availability.push_back(false);
    } else if (part == "1") {
      availability.push_back(true);
    } else {
      return std::nullopt;
    }
  }

  return availability;
}

std::shared_ptr<Board> ParseBoardFromFEN(const std::string& fen) {
  std::vector<std::string> parts = SplitStr(fen, "-");
  if (parts.size() != 7) {
    return nullptr;  // invalid format
  }

  // Not used for teams chess: eliminated players, points
  // Also, currently we don't use the halfmove clock since we don't expect the
  // 50-move rule to apply in real games.

  const auto& player_str = parts[0];
  const auto& castling_availability_kingside = parts[2];
  const auto& castling_availability_queenside = parts[3];
  const auto& piece_placement = parts[6];

  // Parse player
  if (player_str.size() != 1) {
    return nullptr;  // invalid format
  }
  char pchar = player_str[0];
  Player player;
  switch (pchar) {
  case 'R':
    player = Player(RED);
    break;
  case 'B':
    player = Player(BLUE);
    break;
  case 'Y':
    player = Player(YELLOW);
    break;
  case 'G':
    player = Player(GREEN);
    break;
  default:
    return nullptr;  // invalid format
  }

  // Parse castling availability
  std::optional<std::vector<bool>> kingside = ParseCastlingAvailability(
      castling_availability_kingside);
  if (!kingside.has_value()) {
    return nullptr;  // invalid format
  }
  std::optional<std::vector<bool>> queenside = ParseCastlingAvailability(
      castling_availability_queenside);
  if (!queenside.has_value()) {
    return nullptr;  // invalid format
  }

  std::unordered_map<Player, CastlingRights> castling_rights;
  for (int player_color = 0; player_color < 4; player_color++) {
    Player pl(static_cast<PlayerColor>(player_color));
    castling_rights[pl] = CastlingRights(kingside.value()[player_color],
                                         queenside.value()[player_color]);
  }

  // Parse piece placement
  std::vector<std::string> rows = SplitStr(piece_placement, "/");
  if (rows.size() != 14) {
    return nullptr;  // invalid format
  }
  std::unordered_map<BoardLocation, Piece> location_to_piece;
  for (size_t row = 0; row < rows.size(); row++) {
    std::vector<std::string> cols = SplitStr(rows[row], ",");
    int col = 0;
    for (const auto& col_str : cols) {

      if (col_str.empty()) {
        return nullptr;  // invalid format
      }

      char ch = col_str[0];
      if (ch == 'r' || ch == 'b' || ch == 'y' || ch == 'g') {
        // Parse piece
        if (col_str.size() != 2) {
          return nullptr;  // invalid format
        }
        BoardLocation location(row, col);

        PlayerColor player_color;
        switch (ch) {
        case 'r':
          player_color = RED;
          break;
        case 'b':
          player_color = BLUE;
          break;
        case 'y':
          player_color = YELLOW;
          break;
        case 'g':
          player_color = GREEN;
          break;
        default:
          return nullptr;  // invalid format
        }

        PieceType piece_type;
        switch (col_str[1]) {
        case 'P':
          piece_type = PAWN;
          break;
        case 'R':
          piece_type = ROOK;
          break;
        case 'N':
          piece_type = KNIGHT;
          break;
        case 'B':
          piece_type = BISHOP;
          break;
        case 'K':
          piece_type = KING;
          break;
        case 'Q':
          piece_type = QUEEN;
          break;
        default:
          return nullptr;  // invalid format
        }

        Player player(player_color);
        Piece piece(player, piece_type);

        location_to_piece[location] = piece;

        col++;
      } else if (ch == 'x') {
        // parse empty square
        col += 1;
      } else {
        // Parse empty spaces
        std::optional<int> num_empty = ParseInt(col_str);
        if (!num_empty.has_value() || num_empty.value() <= 0) {
          return nullptr;  // invalid format
        }
        col += num_empty.value();
      }

    }
  }

  return std::make_shared<Board>(
      std::move(player), std::move(location_to_piece),
      std::move(castling_rights));
}

void SendInfoMessage(const std::string& message) {
  std::cout << "info string " << message << std::endl;
}

void SendInvalidCommandMessage(const std::string& line) {
  SendInfoMessage("invalid command: '" + line + "'");
}

namespace {

std::optional<std::tuple<size_t, BoardLocation>> ParseLocation(
    const std::string& move_str, size_t start) {
  if (move_str.size() < start + 2) {
    return std::nullopt;
  }
  char c = move_str[start];
  int col = c - 'a';
  if (col < 0 || col >= 14) {
    return std::nullopt;
  }
  start++;
  int row = move_str[start] - '0';
  if (row < 0 || row >= 10) {
    return std::nullopt;
  }
  start++;
  if (start < move_str.size() && std::isdigit(move_str[start])) {
    int digit = move_str[start] - '0';
    row = 10 * row + digit;
    start++;
  }
  return std::make_tuple(start, BoardLocation(row, col));
}

}  // namespace

std::optional<Move> ParseMove(Board& board, const std::string& move_str) {
  // Expected move string format: <letter-col><number-row>
  auto from = ParseLocation(move_str, 0);
  if (!from.has_value()) {
    return std::nullopt;
  }
  auto to = ParseLocation(move_str, std::get<0>(from.value()));
  if (!to.has_value()) {
    return std::nullopt;
  }

  BoardLocation from_loc = std::get<1>(from.value());
  BoardLocation to_loc = std::get<1>(to.value());

  for (const auto& move : board.GetPseudoLegalMoves()) {
    if (move.From() == from_loc && move.To() == to_loc) {
      return move;
    }
  }
  return std::nullopt;
}


}  // namespace chess

