#ifndef _COMMAND_LINE_H_
#define _COMMAND_LINE_H_
// Command line interface for the engine.
// Supports UCI: https://gist.github.com/DOBRO/2592c6dad754ba67e6dcaec8c90165bf

#include <memory>
#include <mutex>
#include <thread>

#include "player.h"
#include "board.h"
#include "utils.h"


namespace chess {

struct EvaluationOptions {
  std::vector<Move> search_moves;
  std::optional<bool> ponder;
  std::optional<int> red_time;
  std::optional<int> blue_time;
  std::optional<int> yellow_time;
  std::optional<int> green_time;
  std::optional<int> red_inc;
  std::optional<int> blue_inc;
  std::optional<int> yellow_inc;
  std::optional<int> green_inc;
  std::optional<int> moves_to_go;
  std::optional<int> depth;
  std::optional<int> nodes;
  std::optional<int> mate;
  std::optional<bool> infinite;
};


class CommandLine {
 public:

  void Run();

 private:
  void EnableDebug(bool enable);
  void StopEvaluation();
  void ResetPlayer();
  void ResetBoard();
  void SetEvaluationOptions(const EvaluationOptions& options);
  void StartEvaluation();
  void MakePonderMove();
  void HandleCommand(
      const std::string& line,
      const std::vector<std::string>& parts);
  void SetBoard(std::shared_ptr<Board> board);

  std::mutex mutex_;

  // Thread used to run evaluation
  std::unique_ptr<std::thread> thread_;

  std::shared_ptr<Board> board_;
  std::shared_ptr<AlphaBetaPlayer> player_;
  std::optional<Move> best_move_;

  bool running_ = true;
  bool debug_ = false;
  EvaluationOptions options_;
  int hash_table_mb_ = 100;
  bool show_current_line_ = true;
};


}  // namespace chess

#endif  // _COMMAND_LINE_H_

