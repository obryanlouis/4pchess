#ifndef _BOARD_H_
#define _BOARD_H_

// Classes for a 4-player teams chess board (chess.com variant).

#include <functional>
#include <memory>
#include <optional>
#include <ostream>
#include <unordered_map>
#include <utility>
#include <vector>
#include <iostream>

namespace chess {

class Board;

constexpr int kNumPieceTypes = 6;

enum PieceType : int8_t {
  PAWN = 0, KNIGHT = 1, BISHOP = 2, ROOK = 3, QUEEN = 4, KING = 5,
  NO_PIECE = 6,
};

// In centipawns
constexpr int kPieceEvaluations[6] = {
  50,     // PAWN
  300,    // KNIGHT
  400,    // BISHOP
  500,    // ROOK
  1000,   // QUEEN
  10000,  // KING (unused)
};

enum PlayerColor : int8_t {
  UNINITIALIZED_PLAYER = -1,
  RED = 0, BLUE = 1, YELLOW = 2, GREEN = 3,
};

enum Team : int8_t {
  RED_YELLOW = 0, BLUE_GREEN = 1, NO_TEAM = 2,
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

}  // namespace chess


template <>
struct std::hash<chess::Player>
{
  std::size_t operator()(const chess::Player& x) const
  {
    return std::hash<int>()(x.GetColor());
  }
};


namespace chess {

//class Piece {
// public:
//  Piece() : piece_type_(NO_PIECE) { }
//
//  Piece(Player player, PieceType piece_type)
//    : player_(std::move(player)),
//      piece_type_(piece_type)
//  { }
//
//  const Player& GetPlayer() const { return player_; }
//  PieceType GetPieceType() const { return piece_type_; }
//  Team GetTeam() const { return GetPlayer().GetTeam(); }
//  PlayerColor GetColor() const { return GetPlayer().GetColor(); }
//  bool operator==(const Piece& other) const {
//    return player_ == other.player_ && piece_type_ == other.piece_type_;
//  }
//  bool operator!=(const Piece& other) const {
//    return !(*this == other);
//  }
//  friend std::ostream& operator<<(
//      std::ostream& os, const Piece& piece);
//
// private:
//  Player player_;
//  PieceType piece_type_;
//};

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

//extern const Piece* kPieceSet[4][6];


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

}  // namespace chess

template <>
struct std::hash<chess::BoardLocation>
{
  std::size_t operator()(const chess::BoardLocation& x) const
  {
    std::size_t hash = 14479 + 14593 * x.GetRow();
    hash += 24439 * x.GetCol();
    return hash;
  }
};

namespace chess {

// Move or capture. Does not include pawn promotion, en-passant, or castling.
class SimpleMove {
 public:
  SimpleMove() = default;

  SimpleMove(BoardLocation from,
             BoardLocation to)
    : from_(std::move(from)),
      to_(std::move(to))
  { }

  bool Present() const { return from_.Present() && to_.Present(); }
  const BoardLocation& From() const { return from_; }
  const BoardLocation& To() const { return to_; }

  bool operator==(const SimpleMove& other) const {
    return from_ == other.from_
        && to_ == other.to_;
  }

  bool operator!=(const SimpleMove& other) const {
    return !(*this == other);
  }

 private:
  BoardLocation from_;
  BoardLocation to_;
};

enum CastlingType {
  KINGSIDE = 0, QUEENSIDE = 1,
};

class CastlingRights {
 public:
  CastlingRights() = default;

  CastlingRights(bool kingside, bool queenside)
    : bits_(0b10000000 | (kingside << 6) | (queenside << 5)) { }
    //: kingside_(kingside), queenside_(queenside) { }

  bool Present() const { return bits_ & (1 << 7); }
  bool Kingside() const { return bits_ & (1 << 6); }
  bool Queenside() const { return bits_ & (1 << 5); }
  //bool Kingside() const { return kingside_; }
  //bool Queenside() const { return queenside_; }

  bool operator==(const CastlingRights& other) const {
    return bits_ == other.bits_;
    //return kingside_ == other.kingside_ && queenside_ == other.queenside_;
  }
  bool operator!=(const CastlingRights& other) const {
    return !(*this == other);
  }

  static CastlingRights kMissingRights;

 private:
  // bit 0: presence
  // bit 1: kingside
  // bit 2: queenside
  int8_t bits_ = 0;

  //bool kingside_ = true;
  //bool queenside_ = true;
};

class Move {
 public:
  Move() = default;

  // Standard move
  Move(BoardLocation from, BoardLocation to,
       Piece standard_capture = Piece::kNoPiece,
       CastlingRights initial_castling_rights = CastlingRights::kMissingRights,
       CastlingRights castling_rights = CastlingRights::kMissingRights)
    : from_(std::move(from)),
      to_(std::move(to)),
      standard_capture_(standard_capture),
      initial_castling_rights_(std::move(initial_castling_rights)),
      castling_rights_(std::move(castling_rights))
  { }

  // Pawn move
  Move(BoardLocation from, BoardLocation to,
       Piece standard_capture,
       BoardLocation en_passant_location,
       Piece en_passant_capture,
       PieceType promotion_piece_type = NO_PIECE)
    : from_(std::move(from)),
      to_(std::move(to)),
      standard_capture_(standard_capture),
      promotion_piece_type_(promotion_piece_type),
      en_passant_location_(en_passant_location),
      en_passant_capture_(en_passant_capture)
  { }

  // Castling
  Move(BoardLocation from, BoardLocation to,
       SimpleMove rook_move,
       CastlingRights initial_castling_rights,
       CastlingRights castling_rights)
    : from_(std::move(from)),
      to_(std::move(to)),
      rook_move_(rook_move),
      initial_castling_rights_(std::move(initial_castling_rights)),
      castling_rights_(std::move(castling_rights))
  { }

  const BoardLocation& From() const { return from_; }
  const BoardLocation& To() const { return to_; }
  bool Present() const { return from_.Present() && to_.Present(); }
  Piece GetStandardCapture() const {
    return standard_capture_;
  }
  bool IsStandardCapture() const {
    return standard_capture_.Present();
  }
  PieceType GetPromotionPieceType() const {
    return promotion_piece_type_;
  }
  const BoardLocation GetEnpassantLocation() const {
    return en_passant_location_;
  }
  Piece GetEnpassantCapture() const {
    return en_passant_capture_;
  }
  SimpleMove GetRookMove() const { return rook_move_; }
  CastlingRights GetInitialCastlingRights() const {
    return initial_castling_rights_;
  }
  CastlingRights GetCastlingRights() const {
    return castling_rights_;
  }

  bool IsCapture() const {
    return standard_capture_.Present() || en_passant_capture_.Present();
  }
  Piece GetCapturePiece() const {
    return standard_capture_.Present() ? standard_capture_ : en_passant_capture_;
  }

  bool operator==(const Move& other) const {
    return from_ == other.from_
        && to_ == other.to_
        && standard_capture_ == other.standard_capture_
        && promotion_piece_type_ == other.promotion_piece_type_
        && en_passant_location_ == other.en_passant_location_
        && en_passant_capture_ == other.en_passant_capture_
        && rook_move_ == other.rook_move_
        && initial_castling_rights_ == other.initial_castling_rights_
        && castling_rights_ == other.castling_rights_;
  }
  bool operator!=(const Move& other) const {
    return !(*this == other);
  }
  int ManhattanDistance() const;
  friend std::ostream& operator<<(
      std::ostream& os, const Move& move);
  std::string PrettyStr() const;
  // NOTE: This does not find discovered checks.
  bool DeliversCheck(Board& board);
  int SEE(Board& board, const int* piece_evaluations);
  int ApproxSEE(Board& board, const int* piece_evaluations);

 private:
  BoardLocation from_;  // 1
  BoardLocation to_;  // 1

  // Capture
  Piece standard_capture_; // 1

  // Promotion
  PieceType promotion_piece_type_ = NO_PIECE; // 1

  // En-passant
  BoardLocation en_passant_location_; // 1
  Piece en_passant_capture_;  // 1

  // For castling moves
  SimpleMove rook_move_; // 2

  // Castling rights before the move
  CastlingRights initial_castling_rights_; // 1

  // Castling rights after the move
  CastlingRights castling_rights_; // 1

  // Cached check
  // -1 means missing, 0/1 store check values
  int8_t delivers_check_ = -1; // 1

  static constexpr int kSeeNotSet = -9999999;

  // Static exchange value of the move
  int see_ = kSeeNotSet;
};

enum GameResult {
  IN_PROGRESS = 0,
  WIN_RY = 1,
  WIN_BG = 2,
  STALEMATE = 3,
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

struct EnpassantInitialization {
  // Indexed by PlayerColor
  std::optional<Move> enp_moves[4] = {std::nullopt, std::nullopt, std::nullopt, std::nullopt};
};

struct MoveBuffer {
  Move* buffer = nullptr;
  size_t pos = 0;
  size_t limit = 0;

  template<class... T>
  void emplace_back(T&&... args) {
    if (pos >= limit) {
      std::cout << "Move buffer overflow" << std::endl;
      abort();
    }
    buffer[pos++] = Move(std::forward<T>(args)...);
  }
};


class Board {
 // Conventions:
 // - Red is on the bottom of the board, blue on the left, yellow on top,
 //   green on the right
 // - Rows go downward from the top
 // - Columns go rightward from the left

 public:
  Board(
      Player turn,
      std::unordered_map<BoardLocation, Piece> location_to_piece,
      std::optional<std::unordered_map<Player, CastlingRights>>
        castling_rights = std::nullopt,
      std::optional<EnpassantInitialization> enp = std::nullopt);

  Board(const Board&) = default;

  size_t GetPseudoLegalMoves2(Move* buffer, size_t limit);

  bool IsKingInCheck(const Player& player) const;
  bool IsKingInCheck(Team team) const;

  GameResult CheckWasLastMoveKingCapture() const;
  GameResult GetGameResult(); // Avoid calling during search.

  Team TeamToPlay() const;
  int PieceEvaluation() const;
  int PieceEvaluation(PlayerColor color) const;
  int MobilityEvaluation();
  int MobilityEvaluation(const Player& player);
  const Player& GetTurn() const { return turn_; }
  bool IsAttackedByTeam(
      Team team,
      const BoardLocation& location) const;

//  std::vector<PlacedPiece> GetAttackers(
//      Team team, const BoardLocation& location,
//      bool return_early = false) const;

  size_t GetAttackers2(
      PlacedPiece* buffer, size_t limit,
      Team team, const BoardLocation& location) const;

  BoardLocation GetKingLocation(PlayerColor color) const;
  bool DeliversCheck(const Move& move);

  const Piece& GetPiece(
      int row, int col) const {
    return location_to_piece_[row][col];
  }
  const Piece& GetPiece(
      const BoardLocation& location) const {
    return GetPiece(location.GetRow(), location.GetCol());
  }
  inline bool IsOnPathBetween(
      const BoardLocation& from,
      const BoardLocation& to,
      const BoardLocation& between) const;
  inline bool DiscoversCheck(
      const BoardLocation& king_location,
      const BoardLocation& move_from,
      const BoardLocation& move_to,
      Team attacking_team) const;

  int64_t HashKey() const { return hash_key_; }

  static std::shared_ptr<Board> CreateStandardSetup();
//  bool operator==(const Board& other) const;
//  bool operator!=(const Board& other) const;
  const CastlingRights& GetCastlingRights(const Player& player);

  void MakeMove(const Move& move);
  void UndoMove();
  bool LastMoveWasCapture() const {
    return !moves_.empty() && moves_.back().GetStandardCapture().Present();
  }
  const Move& GetLastMove() const {
    return moves_.back();
  }
  int NumMoves() const { return moves_.size(); }
  const std::vector<Move>& Moves() { return moves_; }


  void GetPawnMoves2(
      MoveBuffer& moves,
      const BoardLocation& from,
      const Piece& piece) const;
  void GetKnightMoves2(
      MoveBuffer& moves,
      const BoardLocation& from,
      const Piece& piece) const;
  void GetBishopMoves2(
      MoveBuffer& moves,
      const BoardLocation& from,
      const Piece& piece) const;
  void GetRookMoves2(
      MoveBuffer& moves,
      const BoardLocation& from,
      const Piece& piece) const;
  void GetQueenMoves2(
      MoveBuffer& moves,
      const BoardLocation& from,
      const Piece& piece) const;
  void GetKingMoves2(
      MoveBuffer& moves,
      const BoardLocation& from,
      const Piece& piece) const;
  void AddMovesFromIncrMovement2(
      MoveBuffer& moves,
      const Piece& piece,
      const BoardLocation& from,
      int incr_row,
      int incr_col,
      CastlingRights initial_castling_rights = CastlingRights::kMissingRights,
      CastlingRights castling_rights = CastlingRights::kMissingRights) const;


  friend std::ostream& operator<<(
      std::ostream& os, const Board& board);

  // Use with caution: after you set the player you must reset it to its
  // original value before calling UndoMove past the current moves.
  // These functions may be used by things such as null move pruning.
  void SetPlayer(const Player& player) { turn_ = player; }
  void MakeNullMove();
  void UndoNullMove();

  bool IsLegalLocation(int row, int col) const {
    if (row < 0
        || row > GetMaxRow()
        || col < 0
        || col > GetMaxCol()
        || (row < 3 && (col < 3 || col > 10))
        || (row > 10 && (col < 3 || col > 10))) {
      return false;
    }
    return true;
  }
  bool IsLegalLocation(const BoardLocation& location) const {
    return IsLegalLocation(location.GetRow(), location.GetCol());
  }
  const EnpassantInitialization& GetEnpassantInitialization() { return enp_; }
  const std::vector<std::vector<PlacedPiece>>& GetPieceList() { return piece_list_; };

 private:
  void AddMovesFromIncrMovement(
      std::vector<Move>& moves,
      const Piece& piece,
      const BoardLocation& from,
      int incr_row,
      int incr_col,
      CastlingRights initial_castling_rights = CastlingRights::kMissingRights,
      CastlingRights castling_rights = CastlingRights::kMissingRights) const;
  int GetMaxRow() const { return 13; }
  int GetMaxCol() const { return 13; }
  std::optional<CastlingType> GetRookLocationType(
      const Player& player, const BoardLocation& location) const;
  inline void SetPiece(const BoardLocation& location,
                const Piece& piece);
  inline void RemovePiece(const BoardLocation& location);
  inline bool QueenAttacks(
      const BoardLocation& queen_loc,
      const BoardLocation& other_loc) const;
  inline bool RookAttacks(
      const BoardLocation& rook_loc,
      const BoardLocation& other_loc) const;
  inline bool BishopAttacks(
      const BoardLocation& bishop_loc,
      const BoardLocation& other_loc) const;
  inline bool KingAttacks(
      const BoardLocation& king_loc,
      const BoardLocation& other_loc) const;
  inline bool KnightAttacks(
      const BoardLocation& knight_loc,
      const BoardLocation& other_loc) const;
  inline bool PawnAttacks(
      const BoardLocation& pawn_loc,
      PlayerColor pawn_color,
      const BoardLocation& other_loc) const;

  void InitializeHash();
  void UpdatePieceHash(const Piece& piece, const BoardLocation& loc) {
    hash_key_ ^= piece_hashes_[piece.GetColor()][piece.GetPieceType()]
      [loc.GetRow()][loc.GetCol()];
  }
  void UpdateTurnHash(int turn) {
    hash_key_ ^= turn_hashes_[turn];
  }

  Player turn_;

  Piece location_to_piece_[14][14];
  std::vector<std::vector<PlacedPiece>> piece_list_;

  BoardLocation locations_[14][14];

  CastlingRights castling_rights_[4];
  EnpassantInitialization enp_;
  std::vector<Move> moves_; // list of moves from beginning of game
  std::vector<Move> move_buffer_;
  int piece_evaluation_ = 0;
  int player_piece_evaluations_[4] = {0, 0, 0, 0}; // one per player

  int64_t hash_key_ = 0;
  int64_t piece_hashes_[4][6][14][14];
  int64_t turn_hashes_[4];
  BoardLocation king_locations_[4];

  size_t move_buffer_size_ = 300;
  Move move_buffer_2_[300];
};

// Helper functions

Team OtherTeam(Team team);
Team GetTeam(PlayerColor color);
Player GetNextPlayer(const Player& player);
Player GetPreviousPlayer(const Player& player);
Player GetPartner(const Player& player);

// Returns the static exchange evaluation of a capture.
int StaticExchangeEvaluationCapture(
    const int piece_evaluations[6],
    Board& board,
    const Move& move);


}  // namespace chess


#endif  // _BOARD_H_

