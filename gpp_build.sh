mkdir -p bazel-bin
rm -r -f bazel-bin/cli*
g++ -Wall -O3 -g -std=c++20 board.cc player.cc static_exchange.cc cli.cc utils.cc command_line.cc move_picker.cc transposition_table.cc -o bazel-bin/cli
