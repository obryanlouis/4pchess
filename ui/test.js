// test.js
const addon = require('./build/Release/addon');

//const placed_piece = new addon.PlacedPiece(1, 5, 0, 1);
//console.log('placed piece:', placed_piece.debugString());
//const castling_rights = new addon.CastlingRights(0, true, false);
//console.log('castling rights:', castling_rights.debugString());

const board = new addon.Board(
  0,
  [

    // Red
    new addon.PlacedPiece(12, 3,  0, 0),
    new addon.PlacedPiece(12, 4,  0, 0),
    new addon.PlacedPiece(12, 5,  0, 0),
    new addon.PlacedPiece(12, 6,  0, 0),
    new addon.PlacedPiece(12, 7,  0, 0),
    new addon.PlacedPiece(12, 8,  0, 0),
    new addon.PlacedPiece(12, 9,  0, 0),
    new addon.PlacedPiece(12, 10, 0, 0),

    new addon.PlacedPiece(13, 3,  3, 0),
    new addon.PlacedPiece(13, 4,  1, 0),
    new addon.PlacedPiece(13, 5,  2, 0),
    new addon.PlacedPiece(13, 6,  4, 0),
    new addon.PlacedPiece(13, 7,  5, 0),
    new addon.PlacedPiece(13, 8,  2, 0),
    new addon.PlacedPiece(13, 9,  1, 0),
    new addon.PlacedPiece(13, 10, 3, 0),

    // Blue
    new addon.PlacedPiece(3,  1, 0, 1),
    new addon.PlacedPiece(4,  1, 0, 1),
    new addon.PlacedPiece(5,  1, 0, 1),
    new addon.PlacedPiece(6,  1, 0, 1),
    new addon.PlacedPiece(7,  1, 0, 1),
    new addon.PlacedPiece(8,  1, 0, 1),
    new addon.PlacedPiece(9,  1, 0, 1),
    new addon.PlacedPiece(10, 1, 0, 1),

    new addon.PlacedPiece(3, 0, 3, 1),
    new addon.PlacedPiece(4, 0, 1, 1),
    new addon.PlacedPiece(5, 0, 2, 1),
    new addon.PlacedPiece(6, 0, 4, 1),
    new addon.PlacedPiece(7, 0, 5, 1),
    new addon.PlacedPiece(8, 0, 2, 1),
    new addon.PlacedPiece(9, 0, 1, 1),
    new addon.PlacedPiece(10,0, 3, 1),

    // Yellow
    new addon.PlacedPiece(1, 3,  0, 2),
    new addon.PlacedPiece(1, 4,  0, 2),
    new addon.PlacedPiece(1, 5,  0, 2),
    new addon.PlacedPiece(1, 6,  0, 2),
    new addon.PlacedPiece(1, 7,  0, 2),
    new addon.PlacedPiece(1, 8,  0, 2),
    new addon.PlacedPiece(1, 9,  0, 2),
    new addon.PlacedPiece(1, 10, 0, 2),

    new addon.PlacedPiece(0, 3,  3, 2),
    new addon.PlacedPiece(0, 4,  1, 2),
    new addon.PlacedPiece(0, 5,  2, 2),
    new addon.PlacedPiece(0, 6,  5, 2),
    new addon.PlacedPiece(0, 7,  4, 2),
    new addon.PlacedPiece(0, 8,  2, 2),
    new addon.PlacedPiece(0, 9,  1, 2),
    new addon.PlacedPiece(0, 10, 3, 2),

    // Green
    new addon.PlacedPiece(3,  12, 0, 3),
    new addon.PlacedPiece(4,  12, 0, 3),
    new addon.PlacedPiece(5,  12, 0, 3),
    new addon.PlacedPiece(6,  12, 0, 3),
    new addon.PlacedPiece(7,  12, 0, 3),
    new addon.PlacedPiece(8,  12, 0, 3),
    new addon.PlacedPiece(9,  12, 0, 3),
    new addon.PlacedPiece(10, 12, 0, 3),

    new addon.PlacedPiece(3,  13, 3, 3),
    new addon.PlacedPiece(4,  13, 1, 3),
    new addon.PlacedPiece(5,  13, 2, 3),
    new addon.PlacedPiece(6,  13, 5, 3),
    new addon.PlacedPiece(7,  13, 4, 3),
    new addon.PlacedPiece(8,  13, 2, 3),
    new addon.PlacedPiece(9,  13, 1, 3),
    new addon.PlacedPiece(10, 13, 3, 3),

  ],
  [
    new addon.CastlingRights(0, true, true),
    new addon.CastlingRights(1, true, true),
    new addon.CastlingRights(2, true, true),
    new addon.CastlingRights(3, true, true),
  ]);
console.log(board.debugString());

const player = new addon.Player();
var res = player.makeMove(board, 1);
console.log('make move:', res);

//setTimeout(function() {
//  var res = player.makeMove(board, 10);
//  console.log('res after canceled:', res);
//}, 10);
//
//setTimeout(function() {
//  player.cancelEvaluation();
//  console.log('canceled');
//}, 20);


