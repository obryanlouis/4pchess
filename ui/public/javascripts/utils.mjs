import * as board_util from './board.mjs';

export function toFEN(board) {
  // TODO
}

export function toPGN(board) {
  // TODO
}

export function parseBoardFromFEN(fen_str) {
  // TODO
}

export function parseBoardLocation(loc_str) {
  const re = /[a-n]\d+/;
  if (!re.test(loc_str)) {
    throw new Error(`Invalid location string: ${loc_str}`);
  }
  var m = loc_str.match(re)[0];
  var col = 'abcdefghijklmn'.indexOf(m[0]);
  var row = 14 - parseInt(m.substr(1));
  return new board_util.BoardLocation(row, col);
}

function getInitialKingLocation(turn) {
  // NOTE: This could also handle the initial board configuration (standard,
  // old, byg, etc.)
  if (turn.equals(board_util.kRedPlayer)) {
    return new board_util.BoardLocation(13, 7);
  } else if (turn.equals(board_util.kBluePlayer)) {
    return new board_util.BoardLocation(7, 0);
  } else if (turn.equals(board_util.kYellowPlayer)) {
    return new board_util.BoardLocation(0, 6);
  } else if (turn.equals(board_util.kGreenPlayer)) {
    return new board_util.BoardLocation(6, 13);
  }
  throw new Error(`Invalid turn: ${turn}`);
}

function getCastlingLocation(from, turn, kingside) {
  var delta_row = 0;
  var delta_col = 0;

  if (turn.equals(board_util.kRedPlayer)) {
    delta_col = kingside ? 2 : -2;
  } else if (turn.equals(board_util.kBluePlayer)) {
    delta_row = kingside ? 2 : -2;
  } else if (turn.equals(board_util.kYellowPlayer)) {
    delta_col = kingside ? -2: 2;
  } else if (turn.equals(board_util.kGreenPlayer)) {
    delta_row = kingside ? -2: 2;
  } else {
    throw new Error(`Invalid turn: ${turn}`);
  }

  return new board_util.BoardLocation(from.getRow() + delta_row,
                                      from.getCol() + delta_col);
}

function maybeParsePromotionPieceType(move_str) {
  const re = /=([NBRQ])/;
  if (re.test(move_str)) {
    var piece_type = move_str.match(re)[1];
    if (piece_type == 'N') {
      return board_util.KNIGHT;
    } else if (piece_type == 'B') {
      return board_util.BISHOP;
    } else if (piece_type == 'R') {
      return board_util.ROOK;
    } else if (piece_type == 'Q') {
      return board_util.QUEEN;
    }
  }
  return null;
}

export function parseMove(board, move_str) {
  if (move_str == '#' || move_str == 'R' || move_str == 'T') {
    return null;
  }
  var from = null;
  var to = null;
  var promotion_piece_type = null;
  if (move_str == 'O-O') {
    // Castle kingside
    from = getInitialKingLocation(board.getTurn());
    to = getCastlingLocation(from, board.getTurn(), true);
  } else if (move_str == 'O-O-O') {
    // Castle queenside
    from = getInitialKingLocation(board.getTurn());
    to = getCastlingLocation(from, board.getTurn(), false);
  } else {
    var parts = move_str.split(/[-x]/);
    if (parts.length != 2) {
      throw new Error(`Invalid move string: ${move_str}`);
    }
    from = parseBoardLocation(parts[0]);
    to = parseBoardLocation(parts[1]);
    promotion_piece_type = maybeParsePromotionPieceType(move_str);
  }
  var legal_moves = board.getAllLegalMoves();
  for (const move of legal_moves) {
    if (move.getFrom().equals(from)
        && move.getTo().equals(to)
        && promotion_piece_type == move.getPromotionPieceType()) {
      return move;
    }
  }
  throw new Error(`Illegal move: ${move_str}`);
}

export function parseGameFromPGN(pgn_str) {
  // Remove variations
  pgn_str = pgn_str.replaceAll(/\(.*?\)/gs, '');
  var parts = pgn_str.split('\n');
  const re = /^\d+\. (.*)$/;
  var moves = [];
  var piece_types = [];
  // TODO: handle other types of start fen positions
  var board = board_util.Board.CreateStandardSetup();
  var matched_lines = 0;
  for (var part of parts) {
    if (re.test(part)) {
      matched_lines += 1;
      var move_strs = part.match(re)[1].replaceAll('..', '').trim().split(/\s+/);
      if (move_strs.length > 4) {
        console.log('part', part, 'move_strs', move_strs);
        throw new Error(
            `Expected <= 4 moves per line, found ${move_strs.length}`);
      }
      for (var i = 0; i < move_strs.length; i++) {
        var move = parseMove(board, move_strs.at(i));
        if (move != null) {
          piece_types.push(board.getPiece(move.getFrom()).getPieceType());
          board.makeMove(move);
          moves.push(move);
        }
      }
    }
  }
  if (matched_lines == 0) {
    throw new Error('Invalid PGN: no moves found');
  }
  return {'board': board, 'moves': moves, 'piece_types': piece_types};
}

