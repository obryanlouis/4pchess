# 4 Player Chess Engine

This project implements a [4 player](https://www.chess.com/terms/4-player-chess)
teams chess engine.

## What can you do with it?

*  Play against the computer
*  Analyze your games

## Requirements

*  Node JS
*  Bazel

## Play against the computer

1. Initialize node js (one time):

```
cd ui
npm install
```

2. Start the server:

```
node-gyp install
node app.js
```

3. Go to http://localhost:3000

## Code organization

*  `board.h/cc`: Classes representing the board.
*  `player.h/cc`: Classes representing the player.
*  `ui` directory: Related to the website and UI for playing.

### Run tests

```
# All unit tests
bazel test -c opt :all

# Performance test: you'll want to run with --test_output=all
bazel test -c opt speed_test --test_output=all
```

### A/B tests for playing strength

Take a look at `strength_test.cc` and make updates to the config for player
A vs B. Then run the following to see how often the second player wins.

```
bazel run -c opt strength_test
```

You can use this test to validate that changes actually improve the strength.

