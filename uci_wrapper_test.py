import uci_wrapper

#for start_fen in [
#  uci_wrapper.START_FEN_OLD,
#  uci_wrapper.START_FEN_NEW,
#  uci_wrapper.START_FEN_BYG,
#  uci_wrapper.START_FEN_BY,
#  uci_wrapper.START_FEN_RBG,
#]:
#  uci = uci_wrapper.UciWrapper()
#  uci.set_position(start_fen)
#  move = uci.get_best_move(100)
#  assert move is not None
#  print('best move:', move)
#
## Run one game to completion
#uci = uci_wrapper.UciWrapper()
#completed = False
#moves = []
#while len(moves) <= 1000:
#  uci.set_position(uci_wrapper.START_FEN_NEW, moves)
#  move = uci.get_best_move(10)
#  assert move is not None
#  if move == 'gameover':
#    completed = True
#    break
#  moves.append(move)
#
#assert completed
#print(f'Game finished in {len(moves)}')

# Run a specific game.
uci = uci_wrapper.UciWrapper()
uci.set_position('R-0,0,0,0-1,1,1,1-1,1,1,1-0,0,0,0-0-x,x,x,yR,yN,yB,yK,yQ,yB,yN,yR,x,x,x/x,x,x,yP,yP,yP,yP,yP,yP,yP,yP,x,x,x/x,x,x,8,x,x,x/bR,bP,10,gP,gR/bN,bP,10,gP,gN/bB,bP,10,gP,gB/bQ,bP,10,gP,gK/bK,bP,10,gP,gQ/bB,bP,10,gP,gB/bN,bP,10,gP,gN/bR,bP,10,gP,gR/x,x,x,8,x,x,x/x,x,x,rP,rP,rP,rP,rP,rP,rP,rP,x,x,x/x,x,x,rR,rN,rB,rQ,rK,rB,rN,rR,x,x,x')
print(uci.get_best_move(1000))
uci.set_position('Y-0,0,0,0-1,1,1,1-1,1,1,1-0,0,0,0-1-x,x,x,yR,yN,yB,yK,yQ,yB,yN,yR,x,x,x/x,x,x,yP,yP,yP,yP,yP,yP,yP,yP,x,x,x/x,x,x,8,x,x,x/bR,bP,10,gP,gR/bN,bP,10,gP,gN/bB,bP,10,gP,gB/bQ,bP,10,gP,gK/bK,bP,10,gP,gQ/bB,bP,10,gP,gB/1,bP,10,gP,gN/bR,bP,bN,9,gP,gR/x,x,x,4,rP,3,x,x,x/x,x,x,rP,rP,rP,rP,1,rP,rP,rP,x,x,x/x,x,x,rR,rN,rB,rQ,rK,rB,rN,rR,x,x,x')
print(uci.get_best_move(1000))


