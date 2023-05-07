import requests
from urllib.parse import urljoin
from requests.exceptions import ConnectionError, HTTPError, ReadTimeout
from urllib3.exceptions import ProtocolError

try:
    from http.client import RemoteDisconnected
except ImportError:
    from http.client import BadStatusLine as RemoteDisconnected
import backoff

# url enpoints.
ENDPOINTS={
    "arrow":            "?token={}&arrows={}",
    "chat":             "?token={}&chat={}",
    "play":             "?token={}&play={}",
    "play_selfpartner": "?token={}&play={}&playerId={}",
    "resign":           "?token={}&play=R",
    "stream":           "?token={}&stream=1"
}

# https://www.chess.com/variants-test
# https://www.chess.com/variants
TEST_SERVER_URL = 'https://variants.gcp-sandbox.chess-platform.com/bot'
MAIN_SERVER_URL = 'https://variants.gcp-prod.chess.com/bot'

class Api:
    def __init__(self, url: str, token: str, bot_name: str = "?", version: str = "v1.0.0"):
        # declare base vars.
        self.header = {}
        self.url = url
        self.token = token
        self.version = version
        self.session = requests.Session()
        self.set_user_agent(bot_name)

    def is_final(exception):
        return isinstance(exception, HTTPError) and exception.response.status_code < 500

    @backoff.on_exception(backoff.constant,
        (RemoteDisconnected, ConnectionError, ProtocolError, HTTPError, ReadTimeout),
        max_time = 9999999999999,
        interval = 0.1,
        giveup = is_final)
    def api_get(self, path: str):
        # basic api get request.
        url = urljoin(self.url, path)
        response = self.session.get(url, timeout = 2)
        response.raise_for_status()
        return response.json()

    def stream(self):
        # get the stream data.
        url = urljoin(self.url, ENDPOINTS["stream"].format(self.token))
        return requests.get(url, headers = self.header, stream = True)

    def arrow(self, data: str):
        # send game arrows.
        return self.api_get(ENDPOINTS["arrow"].format(self.token, data))

    def play(self, move: str):
        # send the move to play.
        return self.api_get(ENDPOINTS["play"].format(self.token, move))

    def play_selfpartner(self, move: str, botId: str):
        # send the move to play.
        return self.api_get(ENDPOINTS["play_selfpartner"].format(self.token, move, botId))

    def chat(self, message: str):
        # send a message to the chat.
        return self.api_get(ENDPOINTS["chat"].format(self.token, message))

    def resign(self):
        # resign the game.
        return self.api_get(ENDPOINTS["resign"].format(self.token))

    def set_user_agent(self, user: str):
        # set the user agent.
        # chess.com likes it when you set it.
        self.header.update({"User-Agent": "chesscom-bot/{} user:{}".format(self.version, user)})
        self.session.headers.update(self.header)

