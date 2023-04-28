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

export function parseMove(board, move_str) {
  if (move_str == '#' || move_str == 'R' || move_str == 'T') {
    return null;
  }
  var parts = move_str.split(/[-x]/);
  if (parts.length != 2) {
    throw new Error(`Invalid move string: ${move_str}`);
  }
  var from = parseBoardLocation(parts[0]);
  var to = parseBoardLocation(parts[1]);
  var legal_moves = board.getAllLegalMoves();
  for (const move of legal_moves) {
    if (move.getFrom().equals(from)
        && move.getTo().equals(to)) {
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

