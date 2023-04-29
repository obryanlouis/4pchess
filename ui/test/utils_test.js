import * as board_util from '../public/javascripts/board.mjs';
import * as utils from '../public/javascripts/utils.mjs';
import test from 'unit.js';

describe('utils tests', function(){

  it('parse board location', function(){
      var loc = utils.parseBoardLocation('h2');
      var expected_loc = new board_util.BoardLocation(12, 7);
      test.bool(loc.equals(expected_loc)).isTrue();

      var loc = utils.parseBoardLocation('Qh12+');
      var expected_loc = new board_util.BoardLocation(2, 7);
      test.bool(loc.equals(expected_loc)).isTrue();
  });

  it('parse move', function(){
      var board = board_util.Board.CreateStandardSetup();
      var move = utils.parseMove(board, 'h2-h3');
      var from = new board_util.BoardLocation(12, 7);
      var to = new board_util.BoardLocation(11, 7);
      var expected_move = board_util.Move.FromStandardMove(from, to);

      test.bool(move.equals(expected_move)).isTrue();
  });

  it('parse game from pgn', function(){
      var pgn = `
[GameNr "26780486"]
[TimeControl "2+10"]
[Variant "Teams"]
[RuleVariants "EnPassant"]
[StartFen4 "4PC"]
[Red "TeamTitan2"]
[RedElo "1757"]
[Blue "12345avengers"]
[BlueElo "2004"]
[Yellow "TeamTitan1"]
[YellowElo "1757"]
[Green "MisterWish"]
[GreenElo "2499"]
[Result "0-1"]
[Termination "Yellow resigned. 0-1"]
[Site "www.chess.com/variants/4-player-chess/game/26780486"]
[Date "Wed Apr 26 2023 04:27:04 GMT+0000 (Coordinated Universal Time)"]
[CurrentMove "101"]

1. h2-h3 .. b7-c7 .. g13-g12 .. m8-l8
2. Qg1-i3 .. Qa8-d5 .. i13-i12 .. Qn7-m8
3. Qi3-i7 .. Na10-c9 .. Bf14-i11 .. Nn5-l6
4. Qi7-j6 .. Na5-c6 .. Qh14-i13 .. m11-k11
5. Bi1-h2 .. Qd5-g8 .. Qi13-e9 .. Qm8-l9
6. Bh2xNl6 .. Qg8-i8 .. Qe9xQl9 .. Nn10xQl9
7. Bl6-i3 .. Qi8xBi11 .. Ne14-f12 .. Nl9-j10
8. Qj6-h6 .. Nc9-d11 .. h13-h12 .. Nj10-k12
9. Nj1-k3 .. Nd11xNf12+ .. Kg14-g13 .. Nk12-i13
10. Qh6xm6 .. Qi11-h10 .. e13xNf12 .. Ni13-j11
11. Qm6-h11 .. Qh10xQh11 .. g12xQh11 .. Nj11-i13
12. Bi3-g5 .. Ba6xBi14 .. Rd14xBi14 .. Ni13xRk14
13. Ne1-f3 .. b11-d11 .. Nj14-h13 .. k11-j11
14. e2-e3 .. b4-c4 .. Ri14xNk14 .. j11xi12
15. Bf1-e2 .. d11-e11 .. j13xi12 .. Rn11-k11
16. Nf3-h4 .. Ra11-d11 .. f12xe11 .. Rk11-k7
17. Nh4-i6 .. Rd11xd13 .. Rk14-j14 .. Rk7-f7
18. f2-f4 .. Rd13xf13+ .. Kg13-h14 .. Rf7-d7
19. Ni6-k5 .. b10-c10 .. Rj14-j6 .. Kn8-n7
20. Be2-g4 .. Rf13-d13 .. Nh13-i11 .. Rd7-g7
21. Nk5-i6 .. Rd13-d14+ .. Kh14-i13 .. Rg7-g13+
22. Bg5-j8 .. Rd14-g14 .. Ni11-h13 .. Rg13xNh13+
23. Bj8-f12 .. Rg14-h14 .. Ki13-j12 .. Rh13-g13
24. Bf12xRg13 .. Rh14-j14+ .. Kj12-i13 .. m7-k7
25. Ni6-g7 .. Rj14xRj6 .. e11-e10 .. Bn6-m7
26. Ng7-i6 .. Rj6xj2 .. h11-h10 .. m4-l4
27. Nk3-l5 .. Nc6-e5 .. k13-k11 .. Bm7-i3
28. f4xNe5 .. Rj2-j1+ .. e10-e9 .. Bi3xRk1
29. Kh1-h2 .. Ba9-b10 .. e9-e8 .. Bk1-i3+
30. Kh2-g3 .. Rj1xRd1 .. e8-e7 .. Bn9-k6
31. Nl5xRn4 .. Rd1-f1 .. R
      `;
      var res = utils.parseGameFromPGN(pgn);

      test.bool(res['board'] != null).isTrue();
      test.bool(res['moves'] != null).isTrue();
      test.number(res['moves'].length).is(122);

  });

  it('parse game from pgn with move variations', function(){
      var pgn = `
[GameNr "26780489"]
[TimeControl "2+10"]
[Variant "Teams"]
[RuleVariants "EnPassant"]
[StartFen4 "4PC"]
[Red "TeamTitan2"]
[RedElo "1757"]
[Blue "joshuapro2021"]
[BlueElo "2114"]
[Yellow "TeamTitan1"]
[YellowElo "1757"]
[Green "roadrunner18"]
[GreenElo "2568"]
[Result "1-0"]
[Termination "Blue forfeit on time. 1-0"]
[Site "www.chess.com/variants/4-player-chess/game/26780489"]
[Date "Fri Apr 28 2023 03:36:29 GMT+0000 (Coordinated Universal Time)"]
[CurrentMove "19"]

1. h2-h3 .. b9-c9 .. g13-g12 .. m8-l8
2. Qg1-j4 .. b8-d8 .. i13-i12 .. m11-k11
3. Bi1xBa9 .. Qa8xBa9 .. Qh14-e11 .. Qn7-k10
4. Qj4-h6 .. Na5-c6 .. Qe11-g9 .. Nn10-l9
5. Qh6xm6 .. Qa9-b9 .. Bf14xl8 .. Nn5-l6
6. Qm6xNl6 .. Qb9xf13+ ( .. Na10-c11 .. Bl8xm9+ .. Kn8xBm9
7. Ql6-j6+ .. Nc11-e10 .. Qg9-k9 )  .. Kg14xQf13 .. m5xQl6
7. Nj1-k3 .. Na10-c11 .. Bl8xm9+ .. Kn8xBm9
8. Nk3xm4 .. Nc11-e10 .. Qg9-g3+ .. m7-k7
9. Nm4xBn6 .. b7-c7 .. Qg3-g5 .. Rn4xNn6 ( .. Qk10-f10+
10. Nn6-l5 .. Ne10-g11+ .. Kf13-g13 .. Qf10-f13+
11. Nl5xRn4 .. Ba6xg12 .. Kg13-h14 .. Qf13-g14+
12. Ne1-d3 .. Bg12xh13 .. Kh14-i13 .. Qg14xBi14+ ) 
10. e2-e3 .. b4-c4 .. Kf13-g14 .. Qk10-f10
11. Bf1-e2 .. Ba6-b7 .. Qg5-f4 .. Qf10-i13
12. Be2-k8 .. Bb7-a8+ .. Kg14-g13 .. Bn9-m8
13. Bk8xNl9 .. Ne10-g11 .. Nj14-k12 .. m10xBl9
14. Ne1-f3 .. Ng11-e12+ .. d13xNe12 .. Qi13-e9
15. d2-d3 .. b11-d11 .. Qf4-l10+ .. Km9-l8
16. d3xc4 .. T
      `;
      var res = utils.parseGameFromPGN(pgn);

      test.bool(res['board'] != null).isTrue();
      test.bool(res['moves'] != null).isTrue();
      test.number(res['moves'].length).is(61);

  });

  it('parse game from pgn with castling', function(){
      var pgn = `
[Variant "Teams"]
[RuleVariants "EnPassant"]
[CurrentMove "32"]
[TimeControl "2 | 10"]

1. h2-h3 .. b7-c7 .. g13-g12 .. m8-l8
2. Qg1-i3 .. b9-c9 .. Qh14-g13 .. Qn7-m8
3. Ne1-f3 .. Qa8-b9 .. i13-i12 .. m6-l6
4. e2-e3 .. Na10-c11 .. Ne14-f12 .. Nn10-l9
5. Bf1-e2 .. Na5-c6 .. Nj14-k12 .. Nn5-l4
6. O-O-O .. Ba6-b7 .. j13-j12 .. m5-l5
7. Nj1-k3 .. O-O .. Bi14-j13 .. Bn6-m5
8. Bi1-h2 .. Ra6-a7 .. O-O-O .. O-O-O
      `;
      var res = utils.parseGameFromPGN(pgn);

      test.bool(res['board'] != null).isTrue();
      test.bool(res['moves'] != null).isTrue();
      test.number(res['moves'].length).is(32);
  });

  it('parse game from pgn with castling -- other sides', function(){
      var pgn = `
[Variant "Teams"]
[RuleVariants "EnPassant"]
[CurrentMove "18"]
[TimeControl "4 min"]

1. Nj1-i3 .. Na10-c9 .. Ne14-f12 .. Nn10-l9
2. j2-j3 .. b8-c8 .. e13-e12 .. m10-l10
3. Bi1-j2 .. Ba9-c7 .. Bf14-e13 .. Bn9-m10
4. O-O .. Qa8-b8 .. O-O .. O-O
5. g2-g3 .. O-O-O
      `;
      var res = utils.parseGameFromPGN(pgn);

      test.bool(res['board'] != null).isTrue();
      test.bool(res['moves'] != null).isTrue();
      test.number(res['moves'].length).is(18);
  });

  it('parse game from pgn with promotions', function(){
      var pgn = `
[Variant "Teams"]
[RuleVariants "EnPassant"]
[CurrentMove "40"]
[TimeControl "2 | 10"]

1. h2-h4 .. b7-d7 .. g13-g11 .. m8-k8
2. h4-h5 .. d7-e7 .. g11-g10 .. k8-j8
3. h5-h6 .. e7-f7 .. g10-g9 .. j8-i8
4. h6-h7 .. f7-g7 .. g9-g8 .. Nn10-m8
5. h7-h8 .. g7-h7 .. g8-g7 .. Nn5-l6
6. h8-h9 .. h7-i7 .. g7-g6 .. i8-h8
7. h9-h10 .. i7-j7 .. g6-g5 .. h8-g8
8. h10-h11=Q .. j7-k7=N .. g5-g4=B .. g8-f8
9. Nj1-h2 .. Na5-b7 .. Ne14-g13 .. f8-e8
10. Ne1-f3 .. Na10-c9 .. Nj14-i12 .. e8-d8=R
      `;
      var res = utils.parseGameFromPGN(pgn);

      test.bool(res['board'] != null).isTrue();
      test.bool(res['moves'] != null).isTrue();
      test.number(res['moves'].length).is(40);

      // test promotion pieces
      var board = res['board'];

      test.bool(
        board.getPieceRowCol(3, 7)
        .equals(new board_util.Piece(board_util.kRedPlayer, board_util.QUEEN)))
          .isTrue();

      test.bool(
        board.getPieceRowCol(10, 6)
        .equals(new board_util.Piece(board_util.kYellowPlayer, board_util.BISHOP)))
          .isTrue();

      test.bool(
        board.getPieceRowCol(6, 3)
        .equals(new board_util.Piece(board_util.kGreenPlayer, board_util.ROOK)))
          .isTrue();

      test.bool(
        board.getPieceRowCol(7, 10)
        .equals(new board_util.Piece(board_util.kBluePlayer, board_util.KNIGHT)))
          .isTrue();

  });

});
