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


class UciWrapper:

  def __init__(self):
    self._process = self.create_process()

  def create_process(self):
    return subprocess.Popen(
        os.path.join(os.getcwd(), 'bazel-bin/cli'),
        universal_newlines=True,
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        bufsize=1)

  def __del__(self):
    if self._process is not None:
      self._process.terminate()

  def maybe_recreate_process(self):
    if self._process is None or self._process.returncode is not None:
      print('recreate process')
      self._process = self.create_process()

  def set_position(self, fen: str, moves: list[str] | None = None):
    self.maybe_recreate_process()

    fen = fen.replace('\n', '')
    parts = [f'position fen {fen}']
    if moves:
      parts.append('moves')
      parts.extend(moves)
    msg = ' '.join(parts)
    self._process.stdin.write(msg + '\n')

  def get_best_move(
      self,
      time_limit_ms: int,
      pv_callback: Callable[list[str], None] | None = None):
    self.maybe_recreate_process()

    start = time.time()

    buffer_ms = 50
    min_move_ms = 10
    time_limit_ms = max(time_limit_ms - buffer_ms, min_move_ms)

    msg = f'go movetime {time_limit_ms}'
    start = time.time()
    self._process.stdin.write(msg + '\n')

    def communicate(process, response, pv_callback):
      while True:
        line = process.stdout.readline()
        print(line)
        if 'Game completed' in line:
          response['gameover'] = True
        if ' pv ' in line:
          m = re.search('pv (.*?) score', line)
          pv = m.group(1).split()[:4]
          depth = int(re.search('depth (\d+)', line).group(1))
          movetime = int(re.search('time (\d+)', line).group(1))
          # Avoids calling the callback too frequently
          pvcond = (depth >= 15 or (movetime >= 500 and depth >= 10))
          if pvcond and pv_callback is not None:
            pv_callback(pv)
          response['best_move'] = pv[0]
          response['pv'] = pv
        if 'bestmove' in line:
          m = re.search('bestmove (.*)', line)
          response['best_move'] = m.group(1)
          break
        if not line or 'bestmove' in line or 'Game completed' in line:
          break

    response = {}
    t = threading.Thread(
        target=communicate, args=(self._process, response, pv_callback))
    t.start()
    timeout_sec = (time_limit_ms + buffer_ms) / 1000.0 + 5
    t.join(timeout_sec)

    if t.is_alive():
      print('Warning: UCI subprocess timed out')

    if not response.get('gameover') and 'best_move' not in response:
      raise ValueError('Best move not found in process response:', response)
    return response

