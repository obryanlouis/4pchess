cli: board.cc board.h player.cc player.h move_picker.cc move_picker.h utils.cc utils.h transposition_table.cc transposition_table.h cli.cc command_line.cc command_line.h
	g++ -Wall -O3 -std=c++20 board.cc player.cc cli.cc utils.cc command_line.cc move_picker.cc transposition_table.cc -o cli
clean:
	rm -R -f cli
