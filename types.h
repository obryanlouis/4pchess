#ifndef _TYPES_H_
#define _TYPES_H_

#include <cstdint>
#include <memory>
#include <ostream>
#include <iostream>

namespace chess {

enum PieceType : int8_t {
  PAWN = 0, KNIGHT = 1, BISHOP = 2, ROOK = 3, QUEEN = 4, KING = 5,
  NO_PIECE = 6,
};

enum PlayerColor : int8_t {
  UNINITIALIZED_PLAYER = -1,
  RED = 0, BLUE = 1, YELLOW = 2, GREEN = 3,
};

enum Team : int8_t {
  RED_YELLOW = 0, BLUE_GREEN = 1,
};

class Player {
 public:
  Player() : color_(UNINITIALIZED_PLAYER) { }
  explicit Player(PlayerColor color) : color_(color) { }

  PlayerColor GetColor() const { return color_; }
  Team GetTeam() const {
    return (color_ == RED || color_ == YELLOW) ? RED_YELLOW : BLUE_GREEN;
  }
  bool operator==(const Player& other) const {
    return color_ == other.color_;
  }
  bool operator!=(const Player& other) const {
    return !(*this == other);
  }
  friend std::ostream& operator<<(
      std::ostream& os, const Player& player);

 private:
  PlayerColor color_;
};

class BoardLocation {
 public:
  BoardLocation() : loc_(196) {}
  BoardLocation(int8_t row, int8_t col) {
    loc_ = (row < 0 || row >= 14 || col < 0 || col >= 14)
      ? 196 : 14 * row + col;
  }

  bool Present() const { return loc_ < 196; }
  bool Missing() const { return !Present(); }
  int8_t GetRow() const { return loc_ / 14; }
  int8_t GetCol() const { return loc_ % 14; }

  BoardLocation Relative(int8_t delta_row, int8_t delta_col) const {
    return BoardLocation(GetRow() + delta_row, GetCol() + delta_col);
  }

  bool operator==(const BoardLocation& other) const { return loc_ == other.loc_; }
  bool operator!=(const BoardLocation& other) const { return loc_ != other.loc_; }

  friend std::ostream& operator<<(
      std::ostream& os, const BoardLocation& location);
  std::string PrettyStr() const;

  static BoardLocation kNoLocation;

 private:
  // value 0-195: 1 + 14*row + col
  // value 196: not present
  uint8_t loc_;
};

class Piece {
 public:
  Piece() : Piece(false, RED, NO_PIECE) { }

  Piece(bool present, PlayerColor color, PieceType piece_type) {
    bits_ = (((int8_t)present) << 7)
          | (((int8_t)color) << 5)
          | (((int8_t)piece_type) << 2);
  }

  Piece(PlayerColor color, PieceType piece_type)
    : Piece(true, color, piece_type) { }

  Piece(Player player, PieceType piece_type)
    : Piece(true, player.GetColor(), piece_type) { }

  bool Present() const {
    return bits_ & (1 << 7);
  }
  bool Missing() const { return !Present(); }
  PlayerColor GetColor() const {
    return static_cast<PlayerColor>((bits_ & 0b01100000) >> 5);
  }
  PieceType GetPieceType() const {
    return static_cast<PieceType>((bits_ & 0b00011100) >> 2);
  }

  bool operator==(const Piece& other) const { return bits_ == other.bits_; }
  bool operator!=(const Piece& other) const { return bits_ != other.bits_; }

  Player GetPlayer() const { return Player(GetColor()); }
  Team GetTeam() const { return GetPlayer().GetTeam(); }
  friend std::ostream& operator<<(
      std::ostream& os, const Piece& piece);

  static Piece kNoPiece;

 private:
  // bit 0: presence
  // bit 1-2: player
  // bit 3-5: piece type
  int8_t bits_;
};

class PlacedPiece {
 public:
  PlacedPiece() = default;

  PlacedPiece(const BoardLocation& location,
              const Piece& piece)
    : location_(location),
      piece_(piece)
  { }

  const BoardLocation& GetLocation() const { return location_; }
  const Piece& GetPiece() const { return piece_; }
  friend std::ostream& operator<<(
      std::ostream& os, const PlacedPiece& placed_piece);

 private:
  BoardLocation location_;
  Piece piece_;
};

}  // namespace chess


#endif  // _TYPES_H_

