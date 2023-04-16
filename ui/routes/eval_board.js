const path = require('path');
const addon_path = path.join(__dirname, '../build/Release/addon');
const addon = require(addon_path);
const {
  Worker, isMainThread, parentPort, workerData,
} = require('node:worker_threads');

var turn_to_color = {red: 0, blue: 1, yellow: 2, green: 3};
var piece_name_to_type = {'pawn': 0, 'knight': 1, 'bishop': 2, 'rook': 3,
  'queen': 4, 'king': 5};

function parseBoard(req_json) {
  var board_state = req_json['board_state'];

  var player_color = turn_to_color[board_state['turn']];
  if (player_color == null) {
    throw new Error('invalid player: ' + board_state['turn']);
  }
  var placed_pieces = [];
  for (const placed_piece_obj of board_state['board']) {
    var row = placed_piece_obj['row'];
    var col = placed_piece_obj['col'];
    var pieceName = placed_piece_obj['pieceType'];
    var colorName = placed_piece_obj['color'];
    var pieceType = piece_name_to_type[pieceName];
    var color = turn_to_color[colorName];
    if (pieceType == null) {
      throw new Error('invalid piece type: ' + pieceName);
    }
    if (color == null) {
      throw new Error('invalid piece color: ' + color);
    }
    var cpp_piece = new addon.PlacedPiece(row, col, pieceType, color);
    placed_pieces.push(cpp_piece);
  }
  var castling_rights_map = board_state['castling_rights'];
  var castling_rights = [];
  for (let player_name in castling_rights_map) {
    var player_rights = castling_rights_map[player_name];
    var kingside = player_rights['kingside'];
    var queenside = player_rights['queenside'];
    var color = turn_to_color[player_name];
    if (color == null) {
      throw new Error('invalid castling player: ' + player_name);
    }
    var rights = new addon.CastlingRights(color, kingside, queenside);
    castling_rights.push(rights);
  }
  var board = new addon.Board(player_color, placed_pieces, castling_rights);
  return board;
}

var player = new addon.Player();

parentPort.on('message', message => {
  if (message == 'cancel') {
    player.cancelEvaluation();
  }
});


var req_json = workerData;
var board = parseBoard(req_json);
var depth = req_json['search_depth'];
var secs = req_json['secs_per_move'];
if (!Number.isInteger(depth)) {
  throw new Error('invalid search depth: ' + depth);
}

result = player.makeMove(board, depth, secs);
parentPort.postMessage(result);
