"""Wrapper for the UCI engine in python."""

from typing import Callable

import time
import os
import subprocess
import threading
import re

# FENs for starting positions.
START_FEN_RBG = "R-0,0,0,0-1,1,1,1-1,1,1,1-0,0,0,0-0-x,x,x,yR,yN,yB,yK,yQ,yB,yN,yR,x,x,x/x,x,x,yP,yP,yP,yP,yP,yP,yP,yP,x,x,x/x,x,x,8,x,x,x/bR,bP,10,gP,gR/bN,bP,10,gP,gN/bB,bP,10,gP,gB/bQ,bP,10,gP,gK/bK,bP,10,gP,gQ/bB,bP,10,gP,gB/bN,bP,10,gP,gN/bR,bP,10,gP,gR/x,x,x,8,x,x,x/x,x,x,rP,rP,rP,rP,rP,rP,rP,rP,x,x,x/x,x,x,rR,rN,rB,rK,rQ,rB,rN,rR,x,x,x"
START_FEN_NEW = "R-0,0,0,0-1,1,1,1-1,1,1,1-0,0,0,0-0-x,x,x,yR,yN,yB,yK,yQ,yB,yN,yR,x,x,x/x,x,x,yP,yP,yP,yP,yP,yP,yP,yP,x,x,x/x,x,x,8,x,x,x/bR,bP,10,gP,gR/bN,bP,10,gP,gN/bB,bP,10,gP,gB/bQ,bP,10,gP,gK/bK,bP,10,gP,gQ/bB,bP,10,gP,gB/bN,bP,10,gP,gN/bR,bP,10,gP,gR/x,x,x,8,x,x,x/x,x,x,rP,rP,rP,rP,rP,rP,rP,rP,x,x,x/x,x,x,rR,rN,rB,rQ,rK,rB,rN,rR,x,x,x"
START_FEN_BYG = "R-0,0,0,0-1,1,1,1-1,1,1,1-0,0,0,0-0-x,x,x,yR,yN,yB,yQ,yK,yB,yN,yR,x,x,x/x,x,x,yP,yP,yP,yP,yP,yP,yP,yP,x,x,x/x,x,x,8,x,x,x/bR,bP,10,gP,gR/bN,bP,10,gP,gN/bB,bP,10,gP,gB/bQ,bP,10,gP,gK/bK,bP,10,gP,gQ/bB,bP,10,gP,gB/bN,bP,10,gP,gN/bR,bP,10,gP,gR/x,x,x,8,x,x,x/x,x,x,rP,rP,rP,rP,rP,rP,rP,rP,x,x,x/x,x,x,rR,rN,rB,rQ,rK,rB,rN,rR,x,x,x"
START_FEN_OLD = "R-0,0,0,0-1,1,1,1-1,1,1,1-0,0,0,0-0-x,x,x,yR,yN,yB,yK,yQ,yB,yN,yR,x,x,x/x,x,x,yP,yP,yP,yP,yP,yP,yP,yP,x,x,x/x,x,x,8,x,x,x/bR,bP,10,gP,gR/bN,bP,10,gP,gN/bB,bP,10,gP,gB/bK,bP,10,gP,gQ/bQ,bP,10,gP,gK/bB,bP,10,gP,gB/bN,bP,10,gP,gN/bR,bP,10,gP,gR/x,x,x,8,x,x,x/x,x,x,rP,rP,rP,rP,rP,rP,rP,rP,x,x,x/x,x,x,rR,rN,rB,rQ,rK,rB,rN,rR,x,x,x"
START_FEN_BY = "R-0,0,0,0-1,1,1,1-1,1,1,1-0,0,0,0-0-x,x,x,yR,yN,yB,yQ,yK,yB,yN,yR,x,x,x/x,x,x,yP,yP,yP,yP,yP,yP,yP,yP,x,x,x/x,x,x,8,x,x,x/bR,bP,10,gP,gR/bN,bP,10,gP,gN/bB,bP,10,gP,gB/bQ,bP,10,gP,gQ/bK,bP,10,gP,gK/bB,bP,10,gP,gB/bN,bP,10,gP,gN/bR,bP,10,gP,gR/x,x,x,8,x,x,x/x,x,x,rP,rP,rP,rP,rP,rP,rP,rP,x,x,x/x,x,x,rR,rN,rB,rQ,rK,rB,rN,rR,x,x,x"


def get_move_response(process, response, pv_callback, gameover_callback):
  while True:
    line = process.stdout.readline().strip()
    print(line)
    if 'Game completed' in line:
      response['gameover'] = True
      gameover_callback()
    if ' pv ' in line:
      m = re.search(r'pv (.*?) score ([-\d]+)', line)
      pv = m.group(1).split()
      score = m.group(2)
      depth = int(re.search('depth (\d+)', line).group(1))
      movetime = int(re.search('time (\d+)', line).group(1))
      # Avoids calling the callback too frequently
      pvcond = (depth >= 15 or (movetime >= 500 and depth >= 10))
      if pvcond and pv_callback is not None:
        pv_callback(pv)
      response['best_move'] = pv[0]
      response['pv'] = pv
      response['score'] = int(score)
      response['depth'] = depth
    if 'bestmove' in line:
      m = re.search('bestmove (.*)', line)
      response['best_move'] = m.group(1)
      break
    if not line or 'bestmove' in line or 'Game completed' in line:
      break


class UciWrapper:

  def __init__(self, num_threads, max_depth, ponder):
    self._num_threads = num_threads
    self.create_process(num_threads)
    self._max_depth = max_depth
    self._ponder_thread = None
    self._ponder_result = {}
    self._ponder_state = {'ponder_time': 0}
    self._team = None
    self._ponder = ponder

  def create_process(self, num_threads):
    self._process = subprocess.Popen(
        os.path.join(os.getcwd(), 'cli'),
        universal_newlines=True,
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        bufsize=1)
    self._process.stdin.write(
        f'setoption name threads value {num_threads}\n')

  def __del__(self):
    if self._process is not None:
      self._process.terminate()

  def maybe_recreate_process(self):
    if self._process is None or self._process.returncode is not None:
      print('recreate process')
      self._process = self.create_process()
      self.set_team(self._team)

  def maybe_stop_ponder_thread(self):
    if self._ponder_thread is not None:
      self._process.stdin.write('stop\n')
      try:
        self._ponder_thread.join(10e-6)  # make sure it actually stops
      except:
        pass
      self._ponder_thread = None

  def set_team(self, team):
    if team != self._team:
      print('set team:', team)
      self._team = team
      if self._team is not None:
        self._process.stdin.write(
            f'setoption name engine_team value {team}\n')

  def ponder(self, fen: str, move: str, gameover_callback):
    if (self._ponder_thread is not None
        and self._ponder_thread.is_alive()):
      return

    print('Pondering...')
    self.set_position(fen, [move])
    self._process.stdin.write('go\n')
    self._ponder_result = {}
    self._ponder_state['ponder_time'] = 0
    start = time.time()

    def pv_callback(pv):
      duration = time.time() - start
      self._ponder_state['ponder_time'] = duration

    self._ponder_thread = threading.Thread(
        target=get_move_response,
        args=(self._process, self._ponder_result, pv_callback,
              gameover_callback))
    self._ponder_thread.start()

  def set_position(self, fen: str, moves: list[str] | None = None):
    self.maybe_recreate_process()

    fen = fen.replace('\n', '')
    parts = [f'position fen {fen}']
    if moves:
      parts.append('moves')
      parts.extend(moves)
    msg = ' '.join(parts)
    self._process.stdin.write(msg + '\n')

  def get_num_legal_moves(self):
    self._process.stdin.write('get_num_legal_moves\n')
    line = self._process.stdout.readline().strip()
    pattern = 'info string n_legal (\d+)'
    m = re.match(pattern, line)
    if m:
      n_legal = int(m.group(1))
      return n_legal
    return None


  def get_best_move(
      self,
      time_limit_ms: int,
      gameover_callback: Callable[[], None],
      pv_callback: Callable[list[str], None] | None = None,
      last_move: str | None = None):
    self.maybe_recreate_process()
    self.maybe_stop_ponder_thread()

    if (self._ponder
        and last_move is not None
        and 1000 * self._ponder_state['ponder_time'] >= 1.5 * time_limit_ms
        and 'best_move' in self._ponder_result
        and last_move == self._ponder_result['best_move']
        and len(self._ponder_result['pv']) >= 2):
      print('ponder hit')
      response = dict(self._ponder_result)
      response['best_move'] = response['pv'][1]
      response['pv'] = response['pv'][1:]
      response['ponder_hit'] = True
      return response

    # Stop searching early if the move is forced
    n_legal = self.get_num_legal_moves()
    max_depth = self._max_depth
    if n_legal == 1:
      max_depth = 1
      print('Forced move')

    start = time.time()

    buffer_ms = 50
    min_move_ms = 10
    time_limit_ms = max(time_limit_ms - buffer_ms, min_move_ms)

    msg = f'go movetime {time_limit_ms}'
    if max_depth is not None:
      msg += f' depth {max_depth}'
    start = time.time()
    self._process.stdin.write(msg + '\n')

    response = {}
    t = threading.Thread(
        target=get_move_response,
        args=(self._process, response, pv_callback, gameover_callback))
    t.start()
    timeout_sec = (time_limit_ms + buffer_ms) / 1000.0 + 5
    t.join(timeout_sec)

    if t.is_alive():
      print('Warning: UCI subprocess timed out')

    if not response.get('gameover') and 'best_move' not in response:
      raise ValueError('Best move not found in process response:', response)
    return response

