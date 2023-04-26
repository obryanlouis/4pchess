// Classes that keep track of board state and compute legal moves.
// This largely replicates the existing C++ logic so that the frontend isn't
// so reliant on the backend.
// This makes the UI faster and reduces complexity in server calls.

// Enums

export class GameResult {
  static InProgress = new GameResult('in_progress');
  static WinRy = new GameResult('win_ry');
  static WinBg = new GameResult('win_bg');
  static Stalemate = new GameResult('stalemate');

  constructor(name) {
    this.name = name;
  }

  toString() {
    return `GameResult(${this.name})`;
  }

  equals(other) {
    return (typeof this == typeof other
            && this.name == other.name);
  }
}

export const IN_PROGRESS = GameResult.InProgress;
export const WIN_RY = GameResult.WinRy;
export const WIN_BG = GameResult.WinBg;
export const STALEMATE = GameResult.Stalemate;


export class PieceType {
  static Pawn = new PieceType('pawn', '');
  static Knight = new PieceType('knight', 'N');
  static Bishop = new PieceType('bishop', 'B');
  static Rook = new PieceType('rook', 'R');
  static Queen = new PieceType('queen', 'Q');
  static King = new PieceType('king', 'K');

  constructor(name, short_name) {
    this.name = name;
    this.short_name = short_name;
  }

  toString() {
    return `PieceType(${this.name})`;
  }

  equals(other) {
    return (typeof this == typeof other
            && this.name == other.name);
  }
}

export const PAWN = PieceType.Pawn;
export const KNIGHT = PieceType.Knight;
export const BISHOP = PieceType.Bishop;
export const ROOK = PieceType.Rook;
export const QUEEN = PieceType.Queen;
export const KING = PieceType.King;

export class PlayerColor {
  static Red = new PlayerColor('red');
  static Blue = new PlayerColor('blue');
  static Yellow = new PlayerColor('yellow');
  static Green = new PlayerColor('green');

  constructor(name) {
    this.name = name;
  }

  toString() {
    return `PlayerColor(${this.name})`;
  }

  equals(other) {
    return (typeof this == typeof other
            && this.name == other.name);
  }
}

const RED = PlayerColor.Red;
const BLUE = PlayerColor.Blue;
const YELLOW = PlayerColor.Yellow;
const GREEN = PlayerColor.Green;

export class Team {
  static RedYellow = new Team('red_yellow');
  static BlueGreen = new Team('blue_green');

  constructor(name) {
    this.name = name;
  }

  toString() {
    return `Team(${this.name})`;
  }

  equals(other) {
    return (typeof this == typeof other
            && this.name == other.name);
  }
}

const RED_YELLOW = Team.RedYellow;
const BLUE_GREEN = Team.BlueGreen;

export class CastlingType {
  static Kingside = new CastlingType('kingside');
  static Queenside = new CastlingType('queenside');

  constructor(name) {
    this.name = name;
  }

  toString() {
    return `CastlingType(${this.name})`;
  }

  equals(other) {
    return (typeof this == typeof other
            && this.name == other.name);
  }
}

const KINGSIDE = CastlingType.Kingside;
const QUEENSIDE = CastlingType.Queenside;

// Classes

export class Player {
  constructor(color) {
    if (!(color instanceof PlayerColor)) {
      throw new Error('color is not PlayerColor');
    }
    this.color = color;
  }

  getColor() { return this.color; }
  getTeam() {
    if (this.color.equals(PlayerColor.Red)
        || this.color.equals(PlayerColor.Yellow)) {
      return Team.RedYellow;
    }
    return Team.BlueGreen;
  }

  toString() {
    return `Player(${this.color})`;
  }

  equals(other) {
    return (typeof this == typeof other
            && this.color.equals(other.color));
  }
}

export const kRedPlayer = new Player(PlayerColor.Red);
export const kBluePlayer = new Player(PlayerColor.Blue);
export const kYellowPlayer = new Player(PlayerColor.Yellow);
export const kGreenPlayer = new Player(PlayerColor.Green);

export class Piece {
  constructor(player, pieceType) {
    if (!(player instanceof Player)) {
      throw new Error('player is not Player');
    }
    if (!(pieceType instanceof PieceType)) {
      throw new Error('pieceType is not PieceType');
    }
    this.player = player;
    this.pieceType = pieceType;
  }

  getPlayer() { return this.player; }
  getPieceType() { return this.pieceType; }
  getTeam() { return this.player.getTeam(); }
  getColor() { return this.player.getColor(); }

  toString() {
    return `Piece(${this.player}, ${this.pieceType})`;
  }
}

export class BoardLocation {
  constructor(row, col) {
    this.row = row;
    this.col = col;
  }

  getRow() { return this.row; }
  getCol() { return this.col; }
  relative(delta_row, delta_col) {
    return new BoardLocation(this.row + delta_row, this.col + delta_col);
  }
  equals(other) {
    return (typeof this == typeof other
            && this.row == other.row
            && this.col == other.col)
  }

  toString() {
    return `BoardLocation(${this.row}, ${this.col})`;
  }
}

const kRedInitialRookLocationKingside = new BoardLocation(13, 10);
const kRedInitialRookLocationQueenside = new BoardLocation(13, 3);
const kBlueInitialRookLocationKingside = new BoardLocation(10, 0);
const kBlueInitialRookLocationQueenside = new BoardLocation(3, 0);
const kYellowInitialRookLocationKingside = new BoardLocation(0, 3);
const kYellowInitialRookLocationQueenside = new BoardLocation(0, 10);
const kGreenInitialRookLocationKingside = new BoardLocation(3, 13);
const kGreenInitialRookLocationQueenside = new BoardLocation(10, 13);

export class CastlingRights {
  constructor(kingside = true, queenside = true) {
    this.kingside = kingside;
    this.queenside = queenside;
  }

  getKingside() { return this.kingside; }
  getQueenside() { return this.queenside; }

  toString() {
    return `CastlingRights(${this.kingside}, ${this.queenside})`;
  }
}

export class SimpleMove {
  constructor(from, to, piece) {
    this.from = from;
    this.to = to;
    this.piece = piece;
  }

  getFrom() { return this.from; }
  getTo() { return this.to; }
  getPiece() { return this.piece; }

  toString() {
    return `SimpleMove(${this.from}, ${this.to}, ${this.piece})`;
  }
}

export class Move {
  constructor(from, to, standard_capture, initial_castling_rights,
      castling_rights, en_passant_location, en_passant_capture,
      promotion_piece_type, rook_move) {
    this.from = from;
    this.to = to;
    this.standard_capture = standard_capture;
    this.initial_castling_rights = initial_castling_rights;
    this.castling_rights = castling_rights;
    this.en_passant_location = en_passant_location;
    this.en_passant_capture = en_passant_capture;
    this.promotion_piece_type = promotion_piece_type;
    this.rook_move = rook_move;
    this.ends_game = null;
  }

  toString() {
    return `Move(${this.from}, ${this.to}, ${this.standard_capture},
        ${this.initial_castling_rights}, ${this.castling_rights},
        ${this.en_passant_location}, ${this.en_passant_capture},
        ${this.promotion_piece_type}, ${this.rook_move})`;
  }

  getFrom() { return this.from; }
  getTo() { return this.to; }
  getStandardCapture() { return this.standard_capture; }
  getInitialCastlingRights() { return this.initial_castling_rights; }
  getCastlingRights() { return this.castling_rights; }
  getEnpassantLocation() { return this.en_passant_location; }
  getEnpassantCapture() { return this.en_passant_capture; }
  getPromotionPieceType() { return this.promotion_piece_type; }
  getRookMove() { return this.rook_move; }
  getEndsGame() { return this.ends_game; }
  setEndsGame(ends_game) { this.ends_game = ends_game; }
  manhattanDistance() {
    return Math.abs(this.from.getRow() - this.to.getRow())
      + Math.abs(this.from.getCol() - this.to.getCol());
  }
  getCapture() {
    return this.standard_capture != null ? this.standard_capture
         : this.en_passant_capture;
  }

  static FromStandardMove(
      from, to, standard_capture = null, initial_castling_rights = null,
      castling_rights = null) {
    return new Move(from, to, standard_capture, initial_castling_rights,
        castling_rights, null, null, null, null);
  }

  static FromPawnMove(
      from, to, standard_capture, en_passant_location, en_passant_capture,
      promotion_piece_type) {
    return new Move(from, to, standard_capture, null, null,
        en_passant_location, en_passant_capture, promotion_piece_type, null);
  }

  static FromCastlingMove(from, to, rook_move, initial_castling_rights,
      castling_rights) {
    return new Move(from, to, null, initial_castling_rights, castling_rights,
        null, null, null, rook_move);
  }
}

function getOtherTeam(team) {
  return team.equals(RED_YELLOW) ? BLUE_GREEN : RED_YELLOW;
}

var piece_evaluations = {};
piece_evaluations[PAWN] = 1;
piece_evaluations[KNIGHT] = 3;
piece_evaluations[BISHOP] = 4;
piece_evaluations[ROOK] = 5;
piece_evaluations[QUEEN] = 10;
piece_evaluations[KING] = 0;

function getNextPlayer(player) {
  switch (player.getColor()) {
  case RED:
    return kBluePlayer;
  case BLUE:
    return kYellowPlayer;
  case YELLOW:
    return kGreenPlayer;
  case GREEN:
  default:
    return kRedPlayer;
  }
}

export class PlacedPiece {
  constructor(loc, piece) {
    this.loc = loc;
    this.piece = piece;
  }

  getLocation() { return this.loc; }
  getPiece() { return this.piece; }

  equals(other) {
    return (typeof this == typeof other
            && this.loc.equals(other.loc)
            && this.piece.equals(other.piece));
  }

  toString() {
    return `PlacedPiece(${this.loc}, ${this.piece})`;
  }

};

function getPreviousPlayer(player) {
  switch (player.getColor()) {
  case RED:
    return kGreenPlayer;
  case BLUE:
    return kRedPlayer;
  case YELLOW:
    return kBluePlayer;
  case GREEN:
  default:
    return kYellowPlayer;
  }
}


export class Board {
  constructor(turn, location_to_piece, castling_rights = null) {
    this.turn = turn;
    this.location_to_piece = location_to_piece;
    if (castling_rights == null) {
      this.castling_rights = {}
      this.castling_rights[kRedPlayer] = new CastlingRights();
      this.castling_rights[kBluePlayer] = new CastlingRights();
      this.castling_rights[kYellowPlayer] = new CastlingRights();
      this.castling_rights[kGreenPlayer] = new CastlingRights();
    } else {
      this.castling_rights = castling_rights;
    }
    // Moves since start of the game.
    this.moves = [];
    this.piece_evaluation = 0;
    this.piece_list = {};
    this.piece_list[PlayerColor.Red] = [];
    this.piece_list[PlayerColor.Blue] = [];
    this.piece_list[PlayerColor.Yellow] = [];
    this.piece_list[PlayerColor.Green] = [];
    for (let key in location_to_piece) {
      var loc, piece;
      [loc, piece] = location_to_piece[key];
      var evaluation = piece_evaluations[piece.getPieceType()];
      if (piece.getTeam().equals(RED_YELLOW)) {
        this.piece_evaluation += evaluation;
      } else {
        this.piece_evaluation -= evaluation;
      }
      this.piece_list[piece.getColor()].push(new PlacedPiece(loc, piece));
    }
  }

  makeMove(move) {

    // Cases:
    // 1. Move
    // 2. Capture
    // 3. En-passant
    // 4. Promotion
    // 5. Castling (rights, rook move)

    // Capture
    const capture = this.getPiece(move.getTo());
    if (capture != null) {
      this.removePiece(move.getTo());
      var value = piece_evaluations[capture.getPieceType()];
      if (capture.getTeam().equals(RED_YELLOW)) {
        this.piece_evaluation -= value;
      } else {
        this.piece_evaluation += value;
      }
    }

    const promotion_piece_type = move.getPromotionPieceType();
    if (promotion_piece_type != null) { // Promotion
      this.setPiece(move.getTo(), new Piece(this.turn, promotion_piece_type));
    } else { // Move
      const piece = this.getPiece(move.getFrom());
      if (piece == null) {
        throw new Error('piece == null');
      }
      this.setPiece(move.getTo(), piece);
    }
    this.removePiece(move.getFrom());

    // En-passant
    const enpassant_location = move.getEnpassantLocation();
    if (enpassant_location != null) {
      this.removePiece(enpassant_location);
      const enp_capture = move.getEnpassantCapture();
      var value = piece_evaluations[enp_capture.getPieceType()];
      if (enp_capture.getTeam().equals(RED_YELLOW)) {
        this.piece_evaluation -= value;
      } else {
        this.piece_evaluation += value;
      }
    } else {
      // Castling
      const rook_move = move.getRookMove();
      if (rook_move != null) {
        this.setPiece(rook_move.getTo(), this.getPiece(rook_move.getFrom()));
        this.removePiece(rook_move.getFrom());
      }

      // Castling: rights update
      const castling_rights = move.getCastlingRights();
      if (castling_rights != null) {
        this.castling_rights[this.turn] = castling_rights;
      }
    }

    this.turn = getNextPlayer(this.turn);
    this.moves.push(move);
  }

  undoMove() {
    // Cases:
    // 1. Move
    // 2. Capture
    // 3. En-passant
    // 4. Promotion
    // 5. Castling (rights, rook move)

    if (this.moves.length == 0) {
      throw new Error('no moves to undo');
    }
    const move = this.moves.at(-1);
    var turn_before = getPreviousPlayer(this.turn);

    const to = move.getTo();
    const from = move.getFrom();

    // Move the piece back
    const promotion_piece_type = move.getPromotionPieceType();
    if (promotion_piece_type != null) {
      // Handle promotions
      this.setPiece(from, new Piece(turn_before.getColor(), PAWN));
    } else {
      this.setPiece(from, this.getPiece(to));
    }
    this.removePiece(to);

    // Place back captured pieces
    const standard_capture = move.getStandardCapture();
    if (standard_capture != null) {
      this.setPiece(to, standard_capture);
      var value = piece_evaluations[standard_capture.getPieceType()];
      if (standard_capture.getTeam().equals(RED_YELLOW)) {
        this.piece_evaluation += value;
      } else {
        this.piece_evaluation -= value;
      }
    }

    // Place back en-passant pawns
    const enpassant_location = move.getEnpassantLocation();
    if (enpassant_location != null) {
      this.setPiece(enpassant_location, move.getEnpassantCapture());
      var capture = move.getEnpassantCapture();
      var value = piece_evaluations[capture.getPieceType()];
      if (capture.getTeam().equals(RED_YELLOW)) {
        this.piece_evaluation += value;
      } else {
        this.piece_evaluation -= value;
      }
    } else {
      // Castling: rook move
      const rook_move = move.getRookMove();
      if (rook_move != null) {
        this.setPiece(
            rook_move.getFrom(), new Piece(turn_before, ROOK));
        this.removePiece(rook_move.getTo());
      }

      // Castling: rights update
      const initial_castling_rights = move.getInitialCastlingRights();
      if (initial_castling_rights != null) {
        this.castling_rights[turn_before] = initial_castling_rights;
      }
    }

    this.turn = turn_before;
    this.moves.pop();
  }

  setPiece(loc, piece) {
    this.location_to_piece[loc] = [loc, piece];
    // Add to piece_list_
    this.piece_list[piece.getColor()].push(new PlacedPiece(loc, piece));
  }

  removePiece(loc) {
    const piece = this.getPiece(loc);
    delete this.location_to_piece[loc];
    if (piece == null) {
      throw new Error('piece == null');
    }
    var placed_pieces = this.piece_list[piece.getColor()];
    for (var i = 0; i < placed_pieces.length; i++) {
      var placed_piece = placed_pieces.at(i);
      if (placed_piece.getLocation().equals(loc)) {
        placed_pieces.splice(i, 1);
        break;
      }
    }
  }

  getTurn() { return this.turn; }

  isLegalLocationRowCol(row, col) {
    if (row < 0
        || row > 13
        || col < 0
        || col > 13
        || (row < 3 && (col < 3 || col > 10))
        || (row > 10 && (col < 3 || col > 10))) {
      return false;
    }
    return true;
  }

  isLegalLocation(loc) {
    return this.isLegalLocationRowCol(loc.getRow(), loc.getCol());
  }

  getPiece(loc) {
    var entry = this.location_to_piece[loc];
    if (entry != null) {
      return entry[1];
    }
    return null;
  }

  getPieceRowCol(row, col) {
    return this.getPiece(new BoardLocation(row, col));
  }

  addPawnMoves(
      moves,
      from,
      to,
      color,
      capture = null,
      en_passant_location = null,
      en_passant_capture = null) {
    var is_promotion = false;

    const kRedPromotionRow = 3;
    const kYellowPromotionRow = 10;
    const kBluePromotionCol = 10;
    const kGreenPromotionCol = 3;

    switch (color) {
    case RED:
      is_promotion = to.getRow() == kRedPromotionRow;
      break;
    case BLUE:
      is_promotion = to.getCol() == kBluePromotionCol;
      break;
    case YELLOW:
      is_promotion = to.getRow() == kYellowPromotionRow;
      break;
    case GREEN:
      is_promotion = to.getCol() == kGreenPromotionCol;
      break;
    default:
      assert(false);
      break;
    }

    if (is_promotion) {
      moves.push(Move.FromPawnMove(from, to, capture, en_passant_location, en_passant_capture, KNIGHT));
      moves.push(Move.FromPawnMove(from, to, capture, en_passant_location, en_passant_capture, BISHOP));
      moves.push(Move.FromPawnMove(from, to, capture, en_passant_location, en_passant_capture, ROOK));
      moves.push(Move.FromPawnMove(from, to, capture, en_passant_location, en_passant_capture, QUEEN));
    } else {
      moves.push(Move.FromPawnMove(from, to, capture, en_passant_location, en_passant_capture, null));
    }
  }


  getPawnMoves(piece, from) {
    var moves = [];
    var color = piece.getColor();
    var team = piece.getTeam();

    // Move forward
    var delta_rows = 0;
    var delta_cols = 0;
    var not_moved = false;
    switch (color) {
    case PlayerColor.Red:
      delta_rows = -1;
      not_moved = from.getRow() == 12;
      break;
    case PlayerColor.Blue:
      delta_cols = 1;
      not_moved = from.getCol() == 1;
      break;
    case PlayerColor.Yellow:
      delta_rows = 1;
      not_moved = from.getRow() == 1;
      break;
    case PlayerColor.Green:
      delta_cols = -1;
      not_moved = from.getCol() == 12;
      break;
    default:
      assert(false);
      break;
    }

    var to = from.relative(delta_rows, delta_cols);
    if (this.isLegalLocation(to)) {
      var other_piece = this.getPiece(to);
      if (other_piece == null) {
        // Advance once square
        this.addPawnMoves(moves, from, to, piece.getColor());
        // Initial move (advance 2 squares)
        if (not_moved) {
          to = from.relative(delta_rows * 2, delta_cols * 2);
          const other_piece = this.getPiece(to);
          if (other_piece == null) {
            this.addPawnMoves(moves, from, to, piece.getColor());
          }
        }
      } else {
        // En-passant
        if (other_piece.getPieceType().equals(PAWN)
            && piece.getTeam() != other_piece.getTeam()) {
          if (this.moves.length > 0) {
            const last_move = this.moves.at(-1);
            if (last_move.getTo().equals(to)
                && last_move.manhattanDistance() == 2) {
              const moved_from = last_move.getFrom();
              var delta_row = to.getRow() - moved_from.getRow();
              var delta_col = to.getCol() - moved_from.getCol();
              var enpassant_to = moved_from.relative(
                  delta_row / 2, delta_col / 2);
              this.addPawnMoves(moves, from, enpassant_to, piece.getColor(),
                  null, to, other_piece);
            }
          }
        }
      }
    }

    // Non-enpassant capture
    var check_cols = team.equals(RED_YELLOW);
    var capture_row, capture_col;
    for (var incr = 0; incr < 2; ++incr) {
      capture_row = from.getRow() + delta_rows;
      capture_col = from.getCol() + delta_cols;
      if (check_cols) {
        capture_col += incr == 0 ? -1 : 1;
      } else {
        capture_row += incr == 0 ? -1 : 1;
      }
      if (this.isLegalLocationRowCol(capture_row, capture_col)) {
        const other_piece = this.getPieceRowCol(capture_row, capture_col);
        if (other_piece != null
            && !other_piece.getTeam().equals(team)) {
          this.addPawnMoves(
              moves, from, new BoardLocation(capture_row, capture_col),
              piece.getColor(), other_piece);
        }
      }
    }

    return moves;
  }

  getKnightMoves(piece, from) {
    var that = this;
    var moves = [];
    [true, false].forEach(function(pos_row_sign) {
      [1, 2].forEach(function(abs_delta_row) {
        var delta_row = pos_row_sign ? abs_delta_row : -abs_delta_row;
        [true, false].forEach(function(pos_col_sign) {
          var abs_delta_col = abs_delta_row == 1 ? 2 : 1;
          var delta_col = pos_col_sign ? abs_delta_col : -abs_delta_col;
          var to = from.relative(delta_row, delta_col);
          if (that.isLegalLocation(to)) {
            var capture = that.getPiece(to);
            if (capture == null || capture.getTeam() != piece.getTeam()) {
              moves.push(Move.FromStandardMove(from, to, capture));
            }
          }
        });
      });
    });

    return moves;
  }

  addMovesFromIncrMovement(
      moves, piece, from, incr_row, incr_col, initial_castling_rights = null,
      castling_rights = null) {

    var to = from.relative(incr_row, incr_col);
    while (this.isLegalLocation(to)) {
      var capture = this.getPiece(to);
      if (capture == null) {
        moves.push(Move.FromStandardMove(
              from, to, null, initial_castling_rights, castling_rights));
      } else {
        if (capture.getTeam() != piece.getTeam()) {
          moves.push(Move.FromStandardMove(
                from, to, capture, initial_castling_rights, castling_rights));
        }
        break;
      }
      to = to.relative(incr_row, incr_col);
    }

  }

  getBishopMoves(piece, from) {
    var moves = [];
    for (var pos_row = 0; pos_row < 2; ++pos_row) {
      for (var pos_col = 0; pos_col < 2; ++pos_col) {
        this.addMovesFromIncrMovement(
            moves, piece, from, pos_row > 0 ? 1 : -1, pos_col > 0 ? 1 : -1);
      }
    }
    return moves;
  }

  getRookMoves(piece, from) {
    var moves = [];

    // Update castling rights
    var initial_castling_rights = null;
    var castling_rights = null;
    var castling_type = this.getRookLocationType(piece.getPlayer(), from);
    if (castling_type != null) {
      const curr_rights = this.castling_rights[piece.getPlayer()];
      if (curr_rights != null) {
        if (curr_rights.getKingside() || curr_rights.getQueenside()) {
          if (castling_type.equals(KINGSIDE)) {
            if (curr_rights.getKingside()) {
              initial_castling_rights = curr_rights;
              castling_rights = new CastlingRights(false,
                  curr_rights.getQueenside());
            }
          } else {
            if (curr_rights.getQueenside()) {
              initial_castling_rights = curr_rights;
              castling_rights = new CastlingRights(curr_rights.getKingside(),
                  false);
            }
          }
        }
      }
    }

    for (var do_pos_incr = 0; do_pos_incr < 2; ++do_pos_incr) {
      var incr = do_pos_incr > 0 ? 1 : -1;
      for (var do_incr_row = 0; do_incr_row < 2; ++do_incr_row) {
        var incr_row = do_incr_row > 0 ? incr : 0;
        var incr_col = do_incr_row > 0 ? 0 : incr;
        this.addMovesFromIncrMovement(
            moves, piece, from, incr_row, incr_col,
            initial_castling_rights, castling_rights);
      }
    }

    return moves;
  }

  getQueenMoves(piece, from) {
    return this.getBishopMoves(piece, from).concat(
        this.getRookMoves(piece, from));
  }

  getKingMoves(piece, from) {
    var moves = [];

    var initial_castling_rights = null;
    var castling_rights = null;
    const curr_rights = this.castling_rights[piece.getPlayer()];
    if (curr_rights != null) {
      if (curr_rights.getKingside() || curr_rights.getQueenside()) {
        initial_castling_rights = curr_rights;
        castling_rights = new CastlingRights(false, false);
      }
    }

    for (var delta_row = -1; delta_row < 2; ++delta_row) {
      for (var delta_col = -1; delta_col < 2; ++delta_col) {
        if (delta_row == 0 && delta_col == 0) {
          continue;
        }
        var to = from.relative(delta_row, delta_col);
        if (this.isLegalLocation(to)) {
          var capture = this.getPiece(to);
          if (capture == null
              || capture.getTeam() != piece.getTeam()) {
            moves.push(Move.FromStandardMove(
                  from, to, capture, initial_castling_rights,
                  castling_rights));
          }
        }
      }
    }

    if (curr_rights != null) {
      var other_team = getOtherTeam(piece.getTeam());
      for (var is_kingside = 0; is_kingside < 2; ++is_kingside) {
        var allowed = is_kingside > 0 ? curr_rights.getKingside() :
          curr_rights.getQueenside();
        if (allowed) {
          var squares_between = null;
          var rook_location = null;

          switch (piece.getColor()) {
          case RED:
            if (is_kingside) {
              squares_between = [
                from.relative(0, 1),
                from.relative(0, 2),
              ];
              rook_location = from.relative(0, 3);
            } else {
              squares_between = [
                from.relative(0, -1),
                from.relative(0, -2),
                from.relative(0, -3),
              ];
              rook_location = from.relative(0, -4);
            }
            break;
          case BLUE:
            if (is_kingside) {
              squares_between = [
                from.relative(1, 0),
                from.relative(2, 0),
              ];
              rook_location = from.relative(3, 0);
            } else {
              squares_between = [
                from.relative(-1, 0),
                from.relative(-2, 0),
                from.relative(-3, 0),
              ];
              rook_location = from.relative(-4, 0);
            }
            break;
          case YELLOW:
            if (is_kingside) {
              squares_between = [
                from.relative(0, -1),
                from.relative(0, -2),
              ];
              rook_location = from.relative(0, -3);
            } else {
              squares_between = [
                from.relative(0, -1),
                from.relative(0, -2),
                from.relative(0, -3),
              ];
              rook_location = from.relative(0, -4);
            }
            break;
          case GREEN:
            if (is_kingside) {
              squares_between = [
                from.relative(-1, 0),
                from.relative(-2, 0),
              ];
              rook_location = from.relative(-3, 0);
            } else {
              squares_between = [
                from.relative(1, 0),
                from.relative(2, 0),
                from.relative(3, 0),
              ];
              rook_location = from.relative(4, 0);
            }
            break;
          default:
            break;
          }

          // Make sure that the rook is present
          const rook = this.getPiece(rook_location);
          if (rook == null
              || !rook.getPieceType().equals(ROOK)
              || !rook.getTeam().equals(piece.getTeam())) {
            continue;
          }

          // Make sure that there are no pieces between the king and rook
          var piece_between = false;
          for (const loc of squares_between) {
            if (this.getPiece(loc) != null) {
              piece_between = true;
              break;
            }
          }

          if (!piece_between) {
            // Make sure the king is not currently in or would pass through check
            if (!this.isAttackedByTeam(other_team, squares_between.at(0))
                && !this.isAttackedByTeam(other_team, from)) {
              // Additionally move the castle
              const rook = this.getPiece(rook_location);
              var rook_move = new SimpleMove(
                  rook_location, squares_between.at(0), rook);
              moves.push(Move.FromCastlingMove(
                    from, squares_between.at(1), rook_move,
                    initial_castling_rights, castling_rights));
            }
          }
        }
      }
    }

    return moves;
  }

  getKingLocation(turn) {
    for (const placed_piece of this.piece_list[turn.getColor()]) {
      if (placed_piece.getPiece().getColor().equals(turn.getColor())
          && placed_piece.getPiece().getPieceType().equals(KING)) {
        return placed_piece.getLocation();
      }
    }
    return null;
  }

  isAttackedByTeam(team, loc) {
    const loc_row = loc.getRow();
    const loc_col = loc.getCol();

    // Rooks & queens
    for (var do_incr_row = 0; do_incr_row < 2; ++do_incr_row) {
      for (var pos_incr = 0; pos_incr < 2; ++pos_incr) {
        var row_incr = do_incr_row ? (pos_incr ? 1 : -1) : 0;
        var col_incr = do_incr_row ? 0 : (pos_incr ? 1 : -1);
        var row = loc_row + row_incr;
        var col = loc_col + col_incr;
        while (row >= 0 && row < 14 && col >= 0 && col < 14) {
          const piece = this.getPieceRowCol(row, col);
          if (piece != null) {
            if (piece.getTeam().equals(team)
                && (piece.getPieceType().equals(ROOK)
                    || piece.getPieceType().equals(QUEEN))) {
              return true;
            }
            break;
          }
          row += row_incr;
          col += col_incr;
        }
      }
    }

    // Bishops & queens
    for (var pos_row = 0; pos_row < 2; ++pos_row) {
      var row_incr = pos_row ? 1 : -1;
      for (var pos_col = 0; pos_col < 2; ++pos_col) {
        var col_incr = pos_col ? 1 : -1;
        var row = loc_row + row_incr;
        var col = loc_col + col_incr;
        while (this.isLegalLocationRowCol(row, col)) {
          const piece = this.getPieceRowCol(row, col);
          if (piece != null) {
            if (piece.getTeam().equals(team)
                && (piece.getPieceType().equals(BISHOP)
                    || piece.getPieceType().equals(QUEEN))) {
              return true;
            }
            break;
          }
          row += row_incr;
          col += col_incr;
        }
      }
    }

    // Knights
    for (var row_less = 0; row_less < 2; ++row_less) {
      for (var pos_row = 0; pos_row < 2; ++pos_row) {
        var row = loc_row + (row_less ? (pos_row ? 1 : -1) : (pos_row ? 2: -2));
        for (var pos_col = 0; pos_col < 2; ++pos_col) {
          var col = loc_col + (row_less ? (pos_col ? 2: -2): (pos_col ? 1 : -1));
          if (this.isLegalLocationRowCol(row, col)) {
            const piece = this.getPieceRowCol(row, col);
            if (piece != null
                && piece.getTeam().equals(team)
                && piece.getPieceType().equals(KNIGHT)) {
              return true;
            }
          }
        }
      }
    }

    // Pawns
    for (var pos_row = 0; pos_row < 2; ++pos_row) {
      var row = pos_row ? loc_row + 1 : loc_row - 1;
      if (row >= 0 && row < 14) {
        for (var pos_col = 0; pos_col < 2; ++pos_col) {
          var col = pos_col ? loc_col + 1 : loc_col - 1;
          if (col >= 0 && col < 14) {
            const piece = this.getPieceRowCol(row, col);
            if (piece != null
                && piece.getTeam().equals(team)
                && piece.getPieceType().equals(PAWN)) {
              switch (piece.getColor()) {
              case RED:
                if (pos_row) {
                  return true;
                }
                break;
              case BLUE:
                if (!pos_col) {
                  return true;
                }
                break;
              case YELLOW:
                if (!pos_row) {
                  return true;
                }
                break;
              case GREEN:
                if (pos_col) {
                  return true;
                }
                break;
              default:
                assert(false);
                break;
              }
            }
          }
        }
      }
    }

    // Kings
    for (var delta_row = -1; delta_row < 2; ++delta_row) {
      var row = loc_row + delta_row;
      for (var delta_col = -1; delta_col < 2; ++delta_col) {
        if (delta_row == 0 && delta_col == 0) {
          continue;
        }
        var col = loc_col + delta_col;
        if (this.isLegalLocationRowCol(row, col)) {
          const piece = this.getPieceRowCol(row, col);
          if (piece != null
              && piece.getTeam().equals(team)
              && piece.getPieceType().equals(KING)) {
            return true;
          }
        }
      }
    }

    return false;
  }

  getAllLegalMoves() {
    var placed_pieces = this.piece_list[this.turn.getColor()];
    var legal_moves = [];
    for (var i = 0; i < placed_pieces.length; i++) {
      var placed_piece = placed_pieces.at(i);
      legal_moves =
        legal_moves.concat(this.getLegalMoves(placed_piece.getPiece(),
              placed_piece.getLocation()));
    }
    return legal_moves;
  }

  getLegalMoves(piece, from) {
    var moves = this.getMovesForPiece(piece, from);
    var legal_moves = [];
    var turn = this.turn;
    var other_team = getOtherTeam(turn.getTeam());
    for (const move of moves) {
      this.makeMove(move);
      var king_location = this.getKingLocation(turn);
      if (king_location == null) {
        throw new Error('king_location is null');
      }
      if (!this.isAttackedByTeam(other_team, king_location)) {
        legal_moves.push(move);
      }
      this.undoMove();
    }
    return legal_moves;
  }

  getMovesForPiece(piece, from) {
    if (piece.getPieceType().equals(PieceType.Pawn)) {
      return this.getPawnMoves(piece, from);
    } else if (piece.getPieceType().equals(PieceType.Knight)) {
      return this.getKnightMoves(piece, from);
    } else if (piece.getPieceType().equals(PieceType.Bishop)) {
      return this.getBishopMoves(piece, from);
    } else if (piece.getPieceType().equals(PieceType.Rook)) {
      return this.getRookMoves(piece, from);
    } else if (piece.getPieceType().equals(PieceType.Queen)) {
      return this.getQueenMoves(piece, from);
    } else if (piece.getPieceType().equals(PieceType.King)) {
      return this.getKingMoves(piece, from);
    }
  }

  static CreateStandardSetup() {
    var location_to_piece = {};
    var castling_rights = {};
    var piece_types = [
      PieceType.Rook,
      PieceType.Knight,
      PieceType.Bishop,
      PieceType.Queen,
      PieceType.King,
      PieceType.Bishop,
      PieceType.Knight,
      PieceType.Rook,
    ];
    var player_colors = [
      PlayerColor.Red,
      PlayerColor.Blue,
      PlayerColor.Yellow,
      PlayerColor.Green,
    ];

    player_colors.forEach((color, index) => {
      var player = new Player(color);
      castling_rights[player] = new CastlingRights();

      var piece_location;
      var delta_row = 0;
      var delta_col = 0;
      var pawn_offset_row = 0;
      var pawn_offset_col = 0;

      switch (color) {
      case PlayerColor.Red:
        piece_location = new BoardLocation(13, 3);
        delta_col = 1;
        pawn_offset_row = -1;
        break;
      case PlayerColor.Blue:
        piece_location = new BoardLocation(3, 0);
        delta_row = 1;
        pawn_offset_col = 1;
        break;
      case PlayerColor.Yellow:
        piece_location = new BoardLocation(0, 10);
        delta_col = -1;
        pawn_offset_row = 1;
        break;
      case PlayerColor.Green:
        piece_location = new BoardLocation(10, 13);
        delta_row = -1;
        pawn_offset_col = -1;
        break;
      }

      piece_types.forEach((piece_type, index) => {
        var pawn_location = piece_location.relative(
            pawn_offset_row, pawn_offset_col);
        location_to_piece[piece_location] = [
          piece_location, new Piece(player, piece_type)];
        location_to_piece[pawn_location] = [
          pawn_location, new Piece(player, PieceType.Pawn)];
        piece_location = piece_location.relative(delta_row, delta_col);
      });

    });

    return new Board(kRedPlayer, location_to_piece);
  }

  getRookLocationType(player, loc) {
    switch (player.getColor()) {
    case RED:
      if (loc.equals(kRedInitialRookLocationKingside)) {
        return KINGSIDE;
      } else if (loc.equals(kRedInitialRookLocationQueenside)) {
        return QUEENSIDE;
      }
      break;
    case BLUE:
      if (loc.equals(kBlueInitialRookLocationKingside)) {
        return KINGSIDE;
      } else if (loc.equals(kBlueInitialRookLocationQueenside)) {
        return QUEENSIDE;
      }
      break;
    case YELLOW:
      if (loc.equals(kYellowInitialRookLocationKingside)) {
        return KINGSIDE;
      } else if (loc.equals(kYellowInitialRookLocationQueenside)) {
        return QUEENSIDE;
      }
      break;
    case GREEN:
      if (loc.equals(kGreenInitialRookLocationKingside)) {
        return KINGSIDE;
      } else if (loc.equals(kGreenInitialRookLocationQueenside)) {
        return QUEENSIDE;
      }
      break;
    default:
      break;
    }
    return null;
  }

}

