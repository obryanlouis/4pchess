"""Server for playing on chess.com."""

import re
import json
import api
import time
import uci_wrapper


_USE_TEST_SERVER = True
_BOT_NAME = 'Team Titan'
_BOT_VERSION = 'v0.0.0'
_API_KEY_FILENAME = 'api_key_prod.txt'
_SERVER_URL = api.MAIN_SERVER_URL
if _USE_TEST_SERVER:
  _API_KEY_FILENAME = 'api_key_test.txt'
  _SERVER_URL = api.TEST_SERVER_URL


def _read_api_token(filepath: str) -> str:
  """Read the API token from the given filepath."""
  with open(filepath) as f:
    return f.read()


class Pgn4Info:

  def __init__(self, base_time_ms: int, incr_time_ms: int):
    self.base_time_ms = base_time_ms
    self.incr_time_ms = incr_time_ms

  @classmethod
  def FromString(cls, pgn4: str):
    m = re.search('TimeControl "(.*?)\\+(.*?)"', pgn4)
    if not m:
      raise ValueError(f'Could not parse time control from pgn4: {pgn4!r}')
    base_time_mins = float(m.group(1))
    incr_time_secs = float(m.group(2))
    base_time_ms = int(base_time_mins * 60 * 1000)
    incr_time_ms = int(incr_time_secs * 1000)
    return Pgn4Info(base_time_ms, incr_time_ms)


class Server:

  def __init__(self):
    self._token = _read_api_token(_API_KEY_FILENAME)
    self._uci = uci_wrapper.UciWrapper()
    self._api = api.Api(_SERVER_URL, self._token, _BOT_NAME, _BOT_VERSION)
    self._pgn4_info = None
    self._last_arrow_request = None

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
          print('json response:', json_response)
        except:
          print('could not parse json response:', part)
        if json_response is not None:
          self._handle_stream_json(json_response)

  def _handle_stream_json(self, json_response):
    if 'pgn4' in json_response:
      self._pgn4_info = Pgn4Info.FromString(json_response['pgn4'])

    info = json_response.get('info')
    if info:
      info = info.lower()
      if 'Game starting' in info:
        return
      elif 'no game found' in info:
        print('no game found...')
        return
      elif info == "it's your turn":
        if 'fen4' in json_response:
          fen = json_response['fen4']
        else:
          fen = json_response['move']['fen']

        fen = fen.replace('\n', '')
        if 'gameOver' in fen:
          self.clear_arrows()
          return

        if fen == '4PCo':
          fen = uci_wrapper.START_FEN_OLD
        elif fen == '4PC':
          fen = uci_wrapper.START_FEN_NEW
        elif fen == '4PCb':
          fen = uci_wrapper.START_FEN_BY
        elif fen == '4PCn':
          fen = uci_wrapper.START_FEN_BYG
        else:
          # the response an FEN string?
          pass

        self._uci.set_position(fen)

        at_move = json_response.get('move', {}).get('atMove', 0)
        max_move_ms = 30000
        if at_move is not None:
          if at_move < 4:
            max_move_ms = 9000

        clock_ms = float(json_response['clock'])
        assert self._pgn4_info is not None
        buffer_ms = 1000
        move_time_ms = self._pgn4_info.incr_time_ms - buffer_ms
        min_remaining_ms = 30000
        if self._pgn4_info.base_time_ms > min_remaining_ms:
          move_time_ms += (self._pgn4_info.base_time_ms - min_remaining_ms) / 20

        move_time_ms = min(move_time_ms, max_move_ms)

        print('move_time_ms:', move_time_ms)
        res = self._uci.get_best_move(move_time_ms, self.display_arrows)
        if res.get('gameover'):
          print('game over... (should not get here!)')
          return
        print('send move:', res['best_move'])
        play_response = self._api.play(res['best_move'])
        print('play response:', play_response)
      else:
        print(f'unrecognized info: {info!r}...')
        return
    else:
      print('ignore json response (no info)')
      return

  def clear_arrows(self):
    self._api.arrow('clear')

  def display_arrows(self, pv: list[str]):
    parts = ['clear']
    for move in pv:
      parts.append(f'{move}-50')
    request = ','.join(parts)
    if request == self._last_arrow_request:
      return
    self._last_arrow_request = request
    response = self._api.arrow(request)
#    print('arrow response:', response)

  def run(self):

    while True:
      # HTTPResponse
      print('new stream request')
      response = self._api.stream()
      self._read_streaming_response(response)
      response.close()
      time.sleep(0.5)


if __name__ == '__main__':
  Server().run()


