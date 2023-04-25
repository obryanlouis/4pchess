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

namespace chess {

class Board;

constexpr int kNumPieceTypes = 6;

enum PieceType {
  UNINITIALIZED_PIECE = -1,
  PAWN = 0, KNIGHT = 1, BISHOP = 2, ROOK = 3, QUEEN = 4, KING = 5,
};

enum PlayerColor {
  UNINITIALIZED_PLAYER = -1,
  RED = 0, BLUE = 1, YELLOW = 2, GREEN = 3,
};

enum Team {
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

class Piece {
 public:
  Piece() : piece_type_(UNINITIALIZED_PIECE) { }

  Piece(Player player, PieceType piece_type)
    : player_(std::move(player)),
      piece_type_(piece_type)
  { }

  const Player& GetPlayer() const { return player_; }
  PieceType GetPieceType() const { return piece_type_; }
  Team GetTeam() const { return GetPlayer().GetTeam(); }
  PlayerColor GetColor() const { return GetPlayer().GetColor(); }
  bool operator==(const Piece& other) const {
    return player_ == other.player_ && piece_type_ == other.piece_type_;
  }
  bool operator!=(const Piece& other) const {
    return !(*this == other);
  }
  friend std::ostream& operator<<(
      std::ostream& os, const Piece& piece);

 private:
  Player player_;
  PieceType piece_type_;
};


class BoardLocation {
 public:
  BoardLocation() = default;

  BoardLocation(int row, int col) : row_(row), col_(col) { }

  int GetRow() const { return row_; }
  int GetCol() const { return col_; }
  bool operator==(const BoardLocation& other) const {
    return row_ == other.row_ && col_ == other.col_;
  }
  bool operator!=(const BoardLocation& other) const {
    return !(*this == other);
  }

  BoardLocation Relative(int delta_rows, int delta_cols) const {
    return BoardLocation(row_ + delta_rows, col_ + delta_cols);
  }
  friend std::ostream& operator<<(
      std::ostream& os, const BoardLocation& location);

  std::string PrettyStr() const;

 private:
  int row_ = 0;
  int col_ = 0;
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
  SimpleMove(BoardLocation from,
             BoardLocation to,
             const Piece* piece)
    : from_(std::move(from)),
      to_(std::move(to)),
      piece_(piece)
  { }

  const BoardLocation& From() const { return from_; }
  const BoardLocation& To() const { return to_; }
  const Piece* GetPiece() const { return piece_; }

  bool operator==(const SimpleMove& other) const {
    return from_ == other.from_
        && to_ == other.to_
        && piece_ == other.piece_;
  }
  bool operator!=(const SimpleMove& other) const {
    return !(*this == other);
  }

 private:
  BoardLocation from_;
  BoardLocation to_;
  const Piece* piece_;
};

enum CastlingType {
  KINGSIDE = 0, QUEENSIDE = 1,
};

class CastlingRights {
 public:
  CastlingRights() = default;

  CastlingRights(bool kingside, bool queenside)
    : kingside_(kingside), queenside_(queenside) { }

  bool Kingside() const { return kingside_; }
  bool Queenside() const { return queenside_; }

  bool operator==(const CastlingRights& other) const {
    return kingside_ == other.kingside_ && queenside_ == other.queenside_;
  }
  bool operator!=(const CastlingRights& other) const {
    return !(*this == other);
  }

 private:
  bool kingside_ = true;
  bool queenside_ = true;
};

class Move {
 public:
  Move() = default;

  // Standard move
  Move(BoardLocation from, BoardLocation to,
       const Piece* standard_capture = nullptr,
//       std::optional<Piece> standard_capture = std::nullopt,
       std::optional<CastlingRights> initial_castling_rights = std::nullopt,
       std::optional<CastlingRights> castling_rights = std::nullopt)
    : from_(std::move(from)),
      to_(std::move(to)),
      standard_capture_(standard_capture),
      initial_castling_rights_(std::move(initial_castling_rights)),
      castling_rights_(std::move(castling_rights))
  { }

  // Pawn move
  Move(BoardLocation from, BoardLocation to,
       const Piece* standard_capture,
//       std::optional<Piece> standard_capture,
       std::optional<BoardLocation> en_passant_location,
//       std::optional<Piece> en_passant_capture,
       const Piece* en_passant_capture,
       std::optional<PieceType> promotion_piece_type)
    : from_(std::move(from)),
      to_(std::move(to)),
      standard_capture_(standard_capture),
      promotion_piece_type_(promotion_piece_type),
      en_passant_location_(std::move(en_passant_location)),
      en_passant_capture_(en_passant_capture)
  { }

  // Castling
  Move(BoardLocation from, BoardLocation to,
       SimpleMove rook_move,
       std::optional<CastlingRights> initial_castling_rights,
       std::optional<CastlingRights> castling_rights)
    : from_(std::move(from)),
      to_(std::move(to)),
      rook_move_(std::move(rook_move)),
      initial_castling_rights_(std::move(initial_castling_rights)),
      castling_rights_(std::move(castling_rights))
  { }

  const BoardLocation& From() const { return from_; }
  const BoardLocation& To() const { return to_; }
  const Piece* GetStandardCapture() const {
    return standard_capture_;
  }
  const std::optional<PieceType>& GetPromotionPieceType() const {
    return promotion_piece_type_;
  }
  const std::optional<BoardLocation>& GetEnpassantLocation() const {
    return en_passant_location_;
  }
  const Piece* GetEnpassantCapture() const {
    return en_passant_capture_;
  }
  const std::optional<SimpleMove>& GetRookMove() const { return rook_move_; }
  const std::optional<CastlingRights>& GetInitialCastlingRights() const {
    return initial_castling_rights_;
  }
  const std::optional<CastlingRights>& GetCastlingRights() const {
    return castling_rights_;
  }

  bool IsCapture() const {
    return standard_capture_ != nullptr || en_passant_capture_ != nullptr;
  }
  const Piece* GetCapturePiece() const {
    if (standard_capture_ == nullptr) {
      return en_passant_capture_;
    }
    return standard_capture_;
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
  bool DeliversCheck(Board& board);

 private:
  BoardLocation from_;
  BoardLocation to_;

  // Capture
  const Piece* standard_capture_ = nullptr;

  // Promotion
  std::optional<PieceType> promotion_piece_type_;

  // En-passant
  std::optional<BoardLocation> en_passant_location_;
  const Piece* en_passant_capture_ = nullptr;

  // For castling moves
  std::optional<SimpleMove> rook_move_;

  // Castling rights before the move
  std::optional<CastlingRights> initial_castling_rights_;

  // Castling rights after the move
  std::optional<CastlingRights> castling_rights_;

  // Cached check
  std::optional<bool> delivers_check_;
};

enum GameResult {
  IN_PROGRESS = 0,
  WIN_RY = 1,
  WIN_BG = 2,
  STALEMATE = 3,
};

class PlacedPiece {
 public:
  PlacedPiece(const BoardLocation& location,
              const Piece* piece)
    : location_(location),
      piece_(piece)
  { }

  const BoardLocation& GetLocation() const { return location_; }
  const Piece* GetPiece() const { return piece_; }
  friend std::ostream& operator<<(
      std::ostream& os, const PlacedPiece& placed_piece);

 private:
  BoardLocation location_;
  const Piece* piece_ = nullptr;
};

struct EnpassantInitialization {
  // Indexed by PlayerColor
  std::optional<Move> enp_moves[4];
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

  Board(Board&) = default;

  // Returns (IN_PROGRESS, [move_list]) if the game is in progress.
  // Returns (GameResult, []) if the game is over.
//  std::pair<GameResult, std::vector<Move>> GetLegalMoves();

  std::vector<Move> GetPseudoLegalMoves();
  bool IsKingInCheck(const Player& player) const;
  bool IsKingInCheck(Team team) const;

  GameResult CheckWasLastMoveKingCapture() const;
  GameResult GetGameResult(); // Avoid calling during search.

  Team TeamToPlay() const;
  int PieceEvaluation() const;
  int MobilityEvaluation();
  int MobilityEvaluation(const Player& player);
  const Player& GetTurn() const { return turn_; }
  bool IsAttackedByTeam(
      Team team,
      const BoardLocation& location) const;
  std::vector<PlacedPiece> GetAttackers(
      Team team, const BoardLocation& location,
      bool return_early = false) const;
  std::optional<BoardLocation> GetKingLocation(const Player& turn) const;
  bool DeliversCheck(const Move& move);

  const Piece* GetPiece(
      int row, int col) const {
    return location_to_piece_[row][col];
  }
  const Piece* GetPiece(
      const BoardLocation& location) const {
    return location_to_piece_[location.GetRow()][location.GetCol()];
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
  const CastlingRights& GetCastlingRights(const Player& player) const;

  void MakeMove(const Move& move);
  void UndoMove();
  bool LastMoveWasCapture() const {
    return !moves_.empty() && moves_.back().GetStandardCapture() != nullptr;
  }
  const Move& GetLastMove() const {
    return moves_.back();
  }
  int NumMoves() const { return moves_.size(); }


  void GetPawnMoves(
      std::vector<Move>& moves,
      const BoardLocation& from,
      const Piece& piece) const;
  void GetKnightMoves(
      std::vector<Move>& moves,
      const BoardLocation& from,
      const Piece& piece) const;
  void GetBishopMoves(
      std::vector<Move>& moves,
      const BoardLocation& from,
      const Piece& piece) const;
  void GetRookMoves(
      std::vector<Move>& moves,
      const BoardLocation& from,
      const Piece& piece) const;
  void GetQueenMoves(
      std::vector<Move>& moves,
      const BoardLocation& from,
      const Piece& piece) const;
  void GetKingMoves(
      std::vector<Move>& moves,
      const BoardLocation& from,
      const Piece& piece) const;

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
      const std::optional<CastlingRights>& initial_castling_rights = std::nullopt,
      const std::optional<CastlingRights>& castling_rights = std::nullopt) const;
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
    hash_key_ ^= piece_hashes_[
      static_cast<int>(piece.GetPieceType())][loc.GetRow()][loc.GetCol()];
  }
  void UpdateTurnHash(int turn) {
    hash_key_ ^= turn_hashes_[turn];
  }

  Player turn_;

  const Piece* location_to_piece_[14][14];
  // [Player][Piece*]
  std::vector<std::vector<PlacedPiece>> piece_list_;

  BoardLocation locations_[14][14];

  std::unordered_map<Player, CastlingRights> castling_rights_;
  EnpassantInitialization enp_;
  std::vector<Move> moves_; // list of moves from beginning of game
  std::vector<Move> move_buffer_;
  int piece_evaluations_[6]; // one per piece type
  int piece_evaluation_ = 0;

  int64_t hash_key_ = 0;
  int64_t piece_hashes_[6][14][14];
  int64_t turn_hashes_[4];
  BoardLocation king_locations_[4];
};

// Helper functions

Team OtherTeam(Team team);
Team GetTeam(PlayerColor color);
Player GetNextPlayer(const Player& player);
Player GetPreviousPlayer(const Player& player);
Player GetPartner(const Player& player);


}  // namespace chess


#endif  // _BOARD_H_
