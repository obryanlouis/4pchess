// Command line interface for the engine.
// Supports UCI: https://gist.github.com/DOBRO/2592c6dad754ba67e6dcaec8c90165bf

#include "command_line.h"


int main(int argc, char* argv[]) {
  chess::CommandLine command_line;
  command_line.Run();
  return 0;
}

