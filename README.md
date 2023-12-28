# 4 Player Chess Engine

This project implements a [4 player](https://www.chess.com/terms/4-player-chess)
teams chess engine.

## What can you do with it?

*  Play against the computer
*  Analyze your games

## Requirements

*  Node JS
*  Bazel

## Command line / UCI

This project implements the chess UCI [protocol](https://gist.github.com/DOBRO/2592c6dad754ba67e6dcaec8c90165bf).

To build, you have a couple of options:
```
# Option 1 (bazel):
bazel build -c opt cli

# Option 2 (Makefile):
make cli
```

After building, run the program:
```
bazel-bin/cli  # for bazel
# or just `cli` for make
```

Then, analyze a position as below.

### From the start position

```
go
```

### From a position specified by FEN

```
position fen <some FEN>
go
```

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

Use [simplechessmatch](https://github.com/tonyjh/simplechessmatch) to test
the strength of changes.

Example:

```
# A/B engine test with 250 ms per move for quick quality tests
./scm --e1 /Users/louisobryan/tmp/4pchess/cli --e2 /Users/louisobryan/4pchess/cli --fixed 250 --games 500 --threads 10 --fens FENs_4PC_balanced.txt \
    --custom1 "setoption name threads value 1" \
    --custom2 "setoption name threads value 1" \
    --pgn4 /tmp/games.pgn4 \
    --margin 3000 \
    --maxmoves 200
```

It is recommended to test with 5000 ms per move for confidence on the results.

Once the run completes, you can check whether the result is significant
by testing whether the p-value is less than `0.05`. For example,

```
python3 ttest.py --n_wins=243 --n_losses=199 --n_draws=25
# => Ttest_1sampResult(statistic=2.100107728239835, pvalue=0.01811446939295551)
```

