"""Server for playing on chess.com."""

import api
import argparse
import json
import re
import requests
import tablebase
import time
import uci_wrapper

parser = argparse.ArgumentParser(
    prog='Server',
    description='Interacts with chess.com')
def parse_bool(x):
  return x.lower() in ['true', 't', '1']
parser.add_argument('-prod', '--prod', type=parse_bool, required=False,
    default=True)
# might need to set this to another value depending on your computer.
# recommended: max(1, #physical processors - 2)
parser.add_argument('-num_threads', '--num_threads', type=int, required=False,
    default=11)
parser.add_argument('-max_depth', '--max_depth', type=int, required=False,
    default=100)
parser.add_argument('-arrows', '--arrows', type=parse_bool, required=False,
    default=False)
# Enables asymmetric evaluation (anti-human eval)
parser.add_argument(
    '-asymmetric_eval', '--asymmetric_eval', type=parse_bool, required=False,
    default=False)
parser.add_argument(
    '-chat_eval', '--chat_eval', type=parse_bool, required=False,
    default=False)
parser.add_argument(
    '-enable_tablebase', '--enable_tablebase', type=parse_bool, required=False,
    default=True)
parser.add_argument(
    '-ponder', '--ponder', type=parse_bool, required=False,
    default=False)
args = parser.parse_args()


_USE_PROD_SERVER = args.prod
_BOT_NAME = 'Team Titan'
_BOT_VERSION = 'v0.0.0'
if _USE_PROD_SERVER:
  _API_KEY_FILENAME = 'api_key_prod.txt'
  _SERVER_URL = api.MAIN_SERVER_URL
else:
  _API_KEY_FILENAME = 'api_key_test.txt'
  _SERVER_URL = api.TEST_SERVER_URL

_MAX_MOVE_MS = 30000
_MIN_REMAINING_MOVE_MS = 30000
_MIN_MOVE_TIME_MS = 100


def _read_api_token(filepath: str) -> str:
  """Read the API token from the given filepath."""
  with open(filepath) as f:
    return f.read().strip()


def _standardize_move(move: str) -> str:
  move = ''.join([x for x in move if x != '+'])
  separator = '-'
  if 'x' in move:
    separator = 'x'
  parts = move.split(separator)
  for i in range(len(parts)):
    part = parts[i]
    m = re.match('^([a-zA-Z]+)', part)
    if m:
      letters = m.group(1)
      if len(letters) > 1:
        parts[i] = part[len(letters) - 1:]
  return '-'.join(parts)


class Pgn4Info:

  def __init__(self, base_time_ms: int, incr_time_ms: int, delay_time_ms: int,
      last_move: str | None, team: str, played_n_moves: int,
      game_number: str):
    self.base_time_ms = base_time_ms
    self.incr_time_ms = incr_time_ms
    self.delay_time_ms = delay_time_ms
    self.last_move = last_move
    self.team = team
    self.played_n_moves = played_n_moves
    self.game_number = game_number

  @classmethod
  def FromString(cls, pgn4: str):
    m = re.search('TimeControl "(.*?)\\+(.*?)"', pgn4)
    if not m:
      return None
    base_time_mins = float(m.group(1))
    extra_time_secs = m.group(2)
    delay_time_secs = 0
    incr_time_secs = 0
    if extra_time_secs.endswith('D'):
      delay_time_secs = float(extra_time_secs[:-1])
    else:
      incr_time_secs = float(extra_time_secs)

    base_time_ms = int(base_time_mins * 60 * 1000)
    incr_time_ms = int(incr_time_secs * 1000)
    delay_time_ms = int(delay_time_secs * 1000)

    pattern = 'GameNr "(.*?)"'
    game_number = None
    m = re.search(pattern, pgn4, re.DOTALL)
    if m:
      game_number = m.group(1)

    last_move = None
    lines = pgn4.split('\n')
    last_line = lines[-1]
    pattern = r'^(\d+)\.'
    m = re.match(pattern, last_line, re.DOTALL)
    move_number = 0
    if m is not None:
      game_move = int(m.group(1))

      pattern = r'([+\w-]+)\s*(?:{.*?})?\s*(?:\.\.\s*|$)'
      matches = re.findall(pattern, last_line, re.DOTALL)
      if matches is not None:
        move_number = 4 * (game_move - 1) + len(matches)
        last_move = _standardize_move(matches[-1])

    pattern = '(Red|Yellow|Green|Blue) "TeamTitan'
    m = re.search(pattern, pgn4, re.IGNORECASE)
    team = None
    if m:
      color = m.group(1).lower()
      if color in {'red', 'yellow'}:
        team = 'red_yellow'
      elif color in {'blue', 'green'}:
        team = 'blue_green'
      else:
        team = 'no_team'

    return Pgn4Info(base_time_ms, incr_time_ms, delay_time_ms, last_move, team,
        move_number, game_number)


class Server:

  def __init__(self):
    self._token = _read_api_token(_API_KEY_FILENAME)
    self._uci = uci_wrapper.UciWrapper(args.num_threads, args.max_depth,
        args.ponder)
    self._api = api.Api(_SERVER_URL, self._token, _BOT_NAME, _BOT_VERSION)
    self._pgn4_info = None
    self._last_arrow_request = None
    self._gameoverchat = False
    self._game_number = None

  def _read_streaming_response(self, response):
    for content in response.iter_content(chunk_size=None):
      content = content.decode('utf-8')
      if not content:
        continue
      for part in content.splitlines():
        part = part.strip()
        if not part:
          continue
        json_response = None
        try:
          json_response = json.loads(part.strip())
        except:
          pass
        if json_response is not None:
          gameover = self._handle_stream_json(json_response)
          if gameover:
            return

  def _handle_gameover(self):
    if not self._gameoverchat:
      self._gameoverchat = True
      self._api.chat('gg')
      self._uci.maybe_stop_ponder_thread()

  def _handle_stream_json(self, json_response):
#    print(json_response)

    if 'pgn4' in json_response:
      pgn4_info = Pgn4Info.FromString(json_response['pgn4'])
      if pgn4_info is not None:
        self._pgn4_info = pgn4_info
        if args.asymmetric_eval:
          self._uci.set_team(pgn4_info.team)
        if (pgn4_info.game_number is not None
            and self._game_number != pgn4_info.game_number):
          self._gameoverchat = False
          self._game_number = pgn4_info.game_number

    info = json_response.get('info')
    if info:
      info = info.lower()
      if 'game starting' in info:
        return False
      elif 'no game found' in info:
        return False
      elif info == "it's not your turn":
        return False
      elif info == "it's your turn":
        if 'fen4' in json_response:
          fen = json_response['fen4']
        else:
          fen = json_response['move']['fen']

        fen = fen.replace('\n', '')
        if 'gameOver' in fen:
          if args.arrows:
            self.clear_arrows()
          self._handle_gameover()
          return True

        if fen == '4PCo':
          fen = uci_wrapper.START_FEN_OLD
        elif fen == '4PC':
          fen = uci_wrapper.START_FEN_NEW
        elif fen == '4PCb':
          fen = uci_wrapper.START_FEN_BY
        elif fen == '4PCn':
          fen = uci_wrapper.START_FEN_BYG
        else:
          # the response is a FEN string?
          pass

        move = None
        if args.enable_tablebase:
          move = tablebase.FEN_TO_BEST_MOVE.get(fen)
        score = None
        if move is None:
          self._uci.set_position(fen)

          max_move_ms = _MAX_MOVE_MS

          if self._pgn4_info.played_n_moves < 4:
            max_move_ms = 5000

          clock_ms = float(json_response['clock'])
          assert self._pgn4_info is not None
          incr_ms = self._pgn4_info.incr_time_ms
          buffer_ms = 1000
          if incr_ms <= 1000:
            buffer_ms = 250

          move_time_ms = self._pgn4_info.delay_time_ms + incr_ms - buffer_ms
          min_remaining_ms = _MIN_REMAINING_MOVE_MS
          if clock_ms > min_remaining_ms:
            move_time_ms += (clock_ms - min_remaining_ms) / 20

          move_time_ms = min(move_time_ms, max_move_ms)
          move_time_ms = max(move_time_ms, _MIN_MOVE_TIME_MS)

          res = self._uci.get_best_move(
              move_time_ms,
              gameover_callback=self._handle_gameover,
              pv_callback=self.display_arrows,
              last_move=self._pgn4_info.last_move)
          if res.get('gameover'):
            self._handle_gameover()
            return True
          move = res['best_move']
          score = res['score']
          depth = res['depth']

          if args.chat_eval and score is not None:
            team = self._pgn4_info.team
            score_ry = None
            if team == 'red_yellow':
              score_ry = score / 100
            elif team == 'blue_green':
              score_ry = -score / 100
            if score_ry is not None:
              if res.get('ponder_hit', False):
                score_ry = -score_ry
              chat = [f'score: {score_ry:.02f}']
              if depth is not None:
                chat.append(f'depth: {depth}')
              chat = ', '.join(chat)
              self._api.chat(chat)

#        if score is not None and score >= 100000000:
#          if not self._gameoverchat:
#            self._gameoverchat = True
#            self._api.chat('gg')
        play_response = self._api.play(move)

        self._uci.ponder(fen, move, self._handle_gameover)

      else:
        return False
    else:
      return False

  def clear_arrows(self):
    self._api.arrow('clear')

  def display_arrows(self, pv: list[str]):
    if not args.arrows:
      return
    parts = ['clear']
    for move in pv[:4]:
      parts.append(f'{move}-50')
    request = ','.join(parts)
    if request == self._last_arrow_request:
      return
    self._last_arrow_request = request
    response = self._api.arrow(request)

  def run(self):

    timeout = .5
    while True:
      # HTTPResponse
      try:
        response = self._api.stream(timeout)
        self._read_streaming_response(response)
        response.close()
      except Exception as e:
        pass
      time.sleep(0.25)


if __name__ == '__main__':
  Server().run()


