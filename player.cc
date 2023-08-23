#include <tuple>
#include <algorithm>
#include <cmath>
#include <utility>
#include <cassert>
#include <functional>
#include <optional>
#include <iostream>
#include <tuple>
#include <stdexcept>
#include <cstring>
#include <thread>
#include <mutex>

#include "board.h"
#include "player.h"
#include "move_picker.h"

namespace chess {

namespace {

int MoveHash(const Move& move) {
  int hash = 6833;
  hash += move.From().GetRow() * 6397;
  hash += move.From().GetCol() * 6961;
  hash += move.To().GetRow() * 7247;
  hash += move.To().GetCol() * 7349;
  hash = hash % AlphaBetaPlayer::kSearchHashBufferSize;
  return hash;
}

}  // namespace

AlphaBetaPlayer::AlphaBetaPlayer(std::optional<PlayerOptions> options) {
  if (options.has_value()) {
    options_ = options.value();
  }

  piece_evaluations_[PAWN] = 50;
  piece_evaluations_[KNIGHT] = 300;
  piece_evaluations_[BISHOP] = 400;
  piece_evaluations_[ROOK] = 500;
  piece_evaluations_[QUEEN] = 1000;
  piece_evaluations_[KING] = 10000;

  piece_move_order_scores_[PAWN] = 10;
  piece_move_order_scores_[KNIGHT] = 20;
  piece_move_order_scores_[BISHOP] = 30;
  piece_move_order_scores_[ROOK] = 40;
  piece_move_order_scores_[QUEEN] = 50;
  piece_move_order_scores_[KING] = 0;

  king_attacker_values_[PAWN] = 0;
  king_attacker_values_[KNIGHT] = 30;
  king_attacker_values_[BISHOP] = 30;
  king_attacker_values_[ROOK] = 40;
  king_attacker_values_[QUEEN] = 50;
  king_attacker_values_[KING] = 0;

  if (options_.enable_transposition_table) {
    transposition_table_ = std::make_unique<TranspositionTable>(
        options_.transposition_table_size);
  }

  for (int i = 1; i < kMaxPly; i++) {
    reductions_[i] = int(10.0 * std::log(i));
  }

  for (int row = 0; row < 14; row++) {
    for (int col = 0; col < 14; col++) {
      if (row <= 2 || row >= 11 || col <= 2 || col >= 11) {
        location_evaluations_[row][col] = 5;
      } else if (row <= 4 || row >= 9 || col <= 4 || col >= 9) {
        location_evaluations_[row][col] = 10;
      } else {
        location_evaluations_[row][col] = 15;
      }
    }
  }

  king_attack_weight_[0] = 0;
  king_attack_weight_[1] = 0;
  king_attack_weight_[2] = 50;
  king_attack_weight_[3] = 75;
  king_attack_weight_[4] = 88;
  king_attack_weight_[5] = 94;
  king_attack_weight_[6] = 97;
  king_attack_weight_[7] = 99;
  for (int i = 8; i < 30; i++) {
    king_attack_weight_[i] = 100;
  }

  if (options_.enable_piece_square_table) {
    for (int cl = 0; cl < 4; cl++) {
      PlayerColor color = static_cast<PlayerColor>(cl);
      for (int pt = 0; pt < 6; pt++) {
        PieceType piece_type = static_cast<PieceType>(pt);
        bool is_piece = (piece_type == QUEEN || piece_type == ROOK
                         || piece_type == BISHOP || piece_type == KNIGHT);

        for (int row = 0; row < 14; row++) {
          for (int col = 0; col < 14; col++) {
            int table_value = 0;

            if (is_piece) {
              // back rank penalties
              bool back_rank = (color == RED && row == 13)
                            || (color == YELLOW && row == 0)
                            || (color == BLUE && col == 0)
                            || (color == GREEN && col == 13);
              if (back_rank) {
                table_value -= 25;
              }

              // preference for centrality
              float center_dist = std::sqrt((row - 6.5) * (row - 6.5)
                                          + (col - 6.5) * (col - 6.5));
              table_value -= (int)(10 * center_dist);

              // preference for pieces on opponent team's back-3 rank
              if (color == RED || color == YELLOW) {
                if (col < 3 || col >= 11) {
                  table_value += 35;
                }
              } else {
                if (row < 3 || row >= 11) {
                  table_value += 35;
                }
              }

            } else if (piece_type == PAWN) {
              // prefer moving rook pawns twice
              switch (color) {
              case RED:
                if ((col == 3 || col == 10) && row == 10) {
                  table_value += 15;
                }
                break;
              case YELLOW:
                if ((col == 3 || col == 10) && row == 3) {
                  table_value += 15;
                }
                break;
              case BLUE:
                if ((row == 3 || row == 10) && col == 3) {
                  table_value += 15;
                }
                break;
              case GREEN:
                if ((row == 3 || row == 10) && col == 10) {
                  table_value += 15;
                }
                break;
              default:
                break;
              }
            }

            piece_square_table_[color][piece_type][row][col] = table_value;
          }
        }
      }
    }
  }

  if (options_.enable_piece_activation) {
    piece_activation_threshold_[KING] = 999;
    piece_activation_threshold_[PAWN] = 999;
    piece_activation_threshold_[NO_PIECE] = 999;
    piece_activation_threshold_[QUEEN] = 5;
    piece_activation_threshold_[BISHOP] = 5;
    piece_activation_threshold_[KNIGHT] = 3;
    piece_activation_threshold_[ROOK] = 5;
  }

  searching_ = new bool[kMaxPly * kSearchHashBufferSize];
}

AlphaBetaPlayer::~AlphaBetaPlayer() {
  delete[] searching_;
}


ThreadState::ThreadState(
    PlayerOptions options, const Board& board, const PVInfo& pv_info)
  : options_(options), board_(board), pv_info_(pv_info) {
  move_buffer_ = new Move[kBufferPartitionSize * kBufferNumPartitions];
  counter_moves = new Move[14*14*14*14];
}

ThreadState::~ThreadState() {
  delete[] move_buffer_;
  delete[] counter_moves;
}

Move* ThreadState::GetNextMoveBufferPartition() {
  if (buffer_id_ >= kBufferNumPartitions) {
    std::cout << "ThreadState move buffer overflow" << std::endl;
    abort();
  }
  return &move_buffer_[buffer_id_++ * kBufferPartitionSize];
}

void ThreadState::ReleaseMoveBufferPartition() {
  assert(buffer_id_ > 0);
  buffer_id_--;
}

int AlphaBetaPlayer::Reduction(int depth, int move_number) const {
  return (reductions_[depth] * reductions_[move_number] + 1500) / 1000;
}


// Alpha-beta search with nega-max framework.
// https://www.chessprogramming.org/Alpha-Beta
// Returns (nega-max value, best move) pair.
// The best move is nullopt if the game is over.
// If the function returns std::nullopt, then it hit the deadline
// before finishing search and the results should not be used.
std::optional<std::tuple<int, std::optional<Move>>> AlphaBetaPlayer::Search(
    Stack* ss,
    NodeType node_type,
    ThreadState& thread_state,
    int ply,
    int depth,
    int alpha,
    int beta,
    bool maximizing_player,
    int expanded,
    const std::optional<
        std::chrono::time_point<std::chrono::system_clock>>& deadline,
    PVInfo& pvinfo,
    int null_moves,
    bool isCutNode) {
  Board& board = thread_state.GetBoard();
  depth = std::max(depth, 0);
  if (canceled_
      || (deadline.has_value()
        && std::chrono::system_clock::now() >= deadline.value())) {
    return std::nullopt;
  }
  num_nodes_++;
  bool is_root_node = ply == 1;

  bool is_pv_node = node_type != NonPV;
  bool is_tt_pv = false;

  std::optional<Move> tt_move;
  if (options_.enable_transposition_table) {
    int64_t key = board.HashKey();

    const HashTableEntry* tte = transposition_table_->Get(key);
    if (tte != nullptr) {
      if (tte->key == key) { // valid entry
        if (tte->depth >= depth) {
          num_cache_hits_++;
          // at non-PV nodes check for an early TT cutoff
          if (!is_root_node
              && !is_pv_node
              && (tte->bound == EXACT
                || (tte->bound == LOWER_BOUND && tte->score >= beta)
                || (tte->bound == UPPER_BOUND && tte->score <= alpha))
             ) {

            if (tte->move.has_value()
                && !tte->move->IsCapture()) {
              UpdateQuietStats(ss, tte->move.value());
            }

            return std::make_tuple(
                std::min(beta, std::max(alpha, tte->score)), tte->move);
          }
        }
        tt_move = tte->move;
        is_tt_pv = tte->is_pv;
      }
    }

  }
  Player player = board.GetTurn();

  int eval = Evaluate(thread_state, maximizing_player, alpha, beta);

  if (depth <= 0 || ply >= kMaxPly) {

    if (options_.enable_transposition_table) {
      transposition_table_->Save(board.HashKey(), 0, std::nullopt, eval, EXACT, is_pv_node);
    }

    return std::make_tuple(eval, std::nullopt);
  }

  (ss+2)->killers[0] = (ss+2)->killers[1] = Move();
  ss->move_count = 0;

  bool in_check = board.IsKingInCheck(player);
  bool partner_checked = board.IsKingInCheck(GetPartner(player));

//  bool any_player_checked = in_check || partner_checked;
//  if (!any_player_checked) {
//    // Note that the last player is not in check
//    PlayerColor color = static_cast<PlayerColor>((player.GetColor() + 1) % 4);
//    Player other_player(color);
//    any_player_checked = board.IsKingInCheck(other_player);
//  }

  // futility pruning
  if (options_.enable_futility_pruning
      && !in_check
      && !is_pv_node
      && !is_tt_pv
      && depth <= 8
      && eval >= beta
      && eval - 150 * depth >= beta
      && eval < kMateValue) {
    return std::make_tuple(beta, std::nullopt);
  }

  // null move pruning
  if (options_.enable_null_move_pruning
      && !is_root_node // not root
      && !is_pv_node // not a pv node
      && null_moves == 0 // last move wasn't null
      && !in_check // not in check
      && eval >= beta
      && !partner_checked
      ) {
    num_null_moves_tried_++;
    board.MakeNullMove();

    // try the null move with possibly reduced depth
    PVInfo null_pvinfo;
    int r = std::min(depth / 3 + 1, depth - 1);
    //int r = depth / 4 + 1 + std::min((eval - beta)/150, 6);

    auto value_and_move_or = Search(
        ss+1, NonPV, thread_state, ply + 1, depth - r - 1,
        -beta, -beta + 1, !maximizing_player, expanded, deadline, null_pvinfo,
        null_moves + 1);

    board.UndoNullMove();

    // if it failed high, skip this move
    if (value_and_move_or.has_value()
        && -std::get<0>(value_and_move_or.value()) >= beta) {
      num_null_moves_pruned_++;

      if (options_.enable_transposition_table) {
        transposition_table_->Save(board.HashKey(), depth, std::nullopt, beta, LOWER_BOUND, is_pv_node);
      }

      return std::make_tuple(beta, std::nullopt);
    }
  }

  std::optional<Move> best_move;
  int player_color = static_cast<int>(player.GetColor());

  int curr_n_activated = thread_state.NActivated()[player_color];
  int curr_total_moves = thread_state.TotalMoves()[player_color];

  std::optional<Move> pv_move = pvinfo.GetBestMove();
  Move* moves = thread_state.GetNextMoveBufferPartition();
  MovePicker move_picker(
    board,
    pv_move,
    ss->killers,
    piece_evaluations_,
    thread_state.history_heuristic,
    piece_move_order_scores_,
    options_.enable_move_order_checks,
    moves,
    kBufferPartitionSize
   , thread_state.counter_moves
    );

  bool has_legal_moves = false;
  int move_count = 0;
  int quiets = 0;

  // some moves are deferred because another thread is searching them already
  int deferred_index = 0;
  std::vector<Move*> deferred_moves;
  bool processing_deferred = false;

  while (true) {
    Move* move_ptr = nullptr;
    if (!deferred_moves.empty() && deferred_index < deferred_moves.size()) {
      move_ptr = deferred_moves[deferred_index++];
      processing_deferred = true;
    } else {
      move_ptr = move_picker.GetNextMove();
      processing_deferred = false;
      if (move_ptr == nullptr) {
        break;
      }
    }
    Move& move = *move_ptr;

    std::optional<std::tuple<int, std::optional<Move>>> value_and_move_or;

    bool delivers_check = false;

    bool lmr_cond1 =
      options_.enable_late_move_reduction
      && depth > 1
      // (this check is done before move_count is incremented, so use the next)
      && (move_count+1) > 1 + (is_pv_node && ply <= 1)
      && (!is_tt_pv
          || !move.IsCapture()
          || (isCutNode && (ss-1)->move_count > 1))
      && !in_check
      && !partner_checked
         ;

    if (lmr_cond1 || options_.enable_late_move_pruning) {
      // this has to be called before the move is made
      delivers_check = move.DeliversCheck(board);
    }

    bool quiet = !in_check && !move.IsCapture() && !delivers_check
      ;

    if (options_.enable_late_move_pruning
        && alpha > -kMateValue  // don't prune if we're mated
        && quiet
        && !is_tt_pv
        && !is_pv_node
        && quiets >= 1+depth*depth/5
        ) {
      num_lm_pruned_++;
      continue;
    }

    board.MakeMove(move);
    int move_hash = MoveHash(move);
    // hash value to promote threads searching on different branches
    int search_hash = ply*kSearchHashBufferSize + move_hash;
    bool maybe_defer = options_.enable_multithreading
      && is_pv_node
      && !processing_deferred;

    if (maybe_defer) {
      bool searching = searching_[search_hash] = true;

      if (searching) {
        // move kept alive so long as move_picker is
        deferred_moves.push_back(move_ptr);
        // undo move and continue
        searching_[search_hash] = false;
        board.UndoMove();
        continue;
      }
    }

    if (board.CheckWasLastMoveKingCapture() != IN_PROGRESS) {
      board.UndoMove();
      if (maybe_defer) {
        searching_[search_hash] = false;
      }

      alpha = beta; // fail hard
      //value = kMateValue;
      best_move = move;
      pvinfo.SetBestMove(move);
      break;
    }

    if (board.IsKingInCheck(player)) { // invalid move
      board.UndoMove();
      if (maybe_defer) {
        searching_[search_hash] = false;
      }

      continue;
    }

    has_legal_moves = true;

    ss->move_count = move_count++;
    if (quiet) {
      quiets++;
    }

    if (options_.enable_mobility_evaluation
        || options_.enable_piece_activation) {
      UpdateMobilityEvaluation(thread_state, player);
    }

    bool is_pv_move = pv_move.has_value() && pv_move.value() == move;

    std::shared_ptr<PVInfo> child_pvinfo;
    if (is_pv_move && pvinfo.GetChild() != nullptr) {
      child_pvinfo = pvinfo.GetChild();
    } else {
      child_pvinfo = std::make_shared<PVInfo>();
    }

    int e = 0;  // extension

    // check extensions at early moves.
    if (options_.enable_check_extensions
        && in_check
        && move_count < 6
        && expanded < 3) {
      num_check_extensions_++;
      e = 1;
    }

    bool lmr = lmr_cond1
        && !delivers_check;

    if (lmr) {
      num_lmr_searches_++;

      int r = Reduction(depth, move_count + 1);
      r = std::clamp(r, 0, depth - 2);
      value_and_move_or = Search(
          ss+1, NonPV, thread_state, ply + 1, depth - 1 - r + e,
          -alpha-1, -alpha, !maximizing_player, expanded + e,
          deadline, *child_pvinfo, null_moves, true);
      if (value_and_move_or.has_value()) {
        int score = -std::get<0>(value_and_move_or.value());
        if (score > alpha) {  // re-search
          num_lmr_researches_++;
          value_and_move_or = Search(
              ss+1, NonPV, thread_state, ply + 1, depth - 1 + e,
              -beta, -alpha, !maximizing_player, expanded + e,
              deadline, *child_pvinfo, null_moves, !isCutNode);
        }
      }

    } else if (!is_pv_node || move_count > 1) {
      value_and_move_or = Search(
          ss+1, NonPV, thread_state, ply + 1, depth - 1 + e,
          -alpha-1, -alpha, !maximizing_player, expanded + e,
          deadline, *child_pvinfo, null_moves, !isCutNode);
    }

    // For PV nodes only, do a full PV search on the first move or after a fail
    // high (in the latter case search only if value < beta), otherwise let the
    // parent node fail low with value <= alpha and try another move.
    if (is_pv_node && (move_count == 1 || (value_and_move_or.has_value() && (
            -std::get<0>(value_and_move_or.value()) > alpha
            && (is_root_node
              || -std::get<0>(value_and_move_or.value()) < beta))))) {
        value_and_move_or = Search(
            ss+1, PV, thread_state, ply + 1, depth - 1 + e,
            -beta, -alpha, !maximizing_player, expanded + e,
            deadline, *child_pvinfo, null_moves, false);
    }

    board.UndoMove();
    if (options_.enable_multithreading && !processing_deferred) {
      searching_[search_hash] = false;
    }

    if (options_.enable_mobility_evaluation
        || options_.enable_piece_activation) { // reset
      thread_state.NActivated()[player_color] = curr_n_activated;
      thread_state.TotalMoves()[player_color] = curr_total_moves;
    }

    if (!value_and_move_or.has_value()) {
      thread_state.ReleaseMoveBufferPartition();
      return std::nullopt; // timeout
    }
    int score = -std::get<0>(value_and_move_or.value());

    if (score >= beta) {
      alpha = beta;
      best_move = move;
      pvinfo.SetChild(child_pvinfo);
      pvinfo.SetBestMove(move);

      if (!move.IsCapture()) {
        const auto& from = move.From();
        const auto& to = move.To();
        if (options_.enable_history_heuristic) {
          thread_state.history_heuristic[from.GetRow()][from.GetCol()]
            [to.GetRow()][to.GetCol()] += (1 << depth);
        }
        if (options_.enable_counter_move_heuristic) {
          thread_state.counter_moves[from.GetRow()*14*14*14 + from.GetCol()*14*14
            + to.GetRow()*14 + to.GetCol()] = move;
        }
      }

      break; // cutoff
    }
    if (score > alpha) {
      alpha = score;
      best_move = move;
      pvinfo.SetChild(child_pvinfo);
      pvinfo.SetBestMove(move);
    }
    if (!best_move.has_value()) {
      best_move = move;
      pvinfo.SetChild(child_pvinfo);
      pvinfo.SetBestMove(move);
    }
  }

  int score = alpha;
  if (!has_legal_moves) {
    if (!in_check) {
      // stalemate
      score = std::min(beta, std::max(alpha, 0));
    } else {
      // checkmate
      score = std::min(beta, std::max(alpha, -kMateValue));
    }
  }

  if (options_.enable_transposition_table) {
    ScoreBound bound = beta <= alpha ? LOWER_BOUND : is_pv_node &&
      best_move.has_value() ? EXACT : UPPER_BOUND;
    transposition_table_->Save(board.HashKey(), depth, best_move, score, bound, is_pv_node);
  }

  if (best_move.has_value()
      && !best_move->IsCapture()) {
    UpdateQuietStats(ss, *best_move);
  }

  // If no good move is found and the previous position was tt_pv, then the
  // previous opponent move is probably good and the new position is added to
  // the search tree.
  if (score <= alpha) {
    ss->tt_pv = ss->tt_pv || ((ss-1)->tt_pv && depth > 3);
  }

  thread_state.ReleaseMoveBufferPartition();
  return std::make_tuple(score, best_move);
}

void AlphaBetaPlayer::UpdateQuietStats(Stack* ss, const Move& move) {
  if (options_.enable_killers) {
    if (ss->killers[0] != move) {
      ss->killers[1] = ss->killers[0];
      ss->killers[0] = move;
    }
  }
}

namespace {

constexpr int kPieceImbalanceTable[16] = {
  0, -50, -100, -300, -600, -700, -800, -800,
  -800, -800, -800, -800, -800, -800, -800, -800,
};

int GetNumMajorPieces(const std::vector<PlacedPiece>& pieces) {
  int num_major = 0;
  for (const auto& placed_piece : pieces) {
    PieceType pt = placed_piece.GetPiece().GetPieceType();
    if (pt != PAWN && pt != KING) {
      num_major++;
    }
  }
  return num_major;
}

}  // namespace

int AlphaBetaPlayer::Evaluate(
    ThreadState& thread_state, bool maximizing_player, int alpha, int beta) {
  int eval; // w.r.t. RY team
  Board& board = thread_state.GetBoard();
  GameResult game_result = board.CheckWasLastMoveKingCapture();
  if (game_result != IN_PROGRESS) { // game is over
    if (game_result == WIN_RY) {
      eval = kMateValue;
    } else if (game_result == WIN_BG) {
      eval = -kMateValue;
    } else {
      eval = 0; // stalemate
    }
  } else {
    // Piece evaluation
    eval = board.PieceEvaluation();

    if (options_.enable_piece_square_table) {
      const auto& piece_list = board.GetPieceList();
      for (int color = 0; color < 4; color++) {
        for (const auto& placed_piece : piece_list[color]) {
          PieceType piece_type = placed_piece.GetPiece().GetPieceType();
          const auto& loc = placed_piece.GetLocation();
          int row = loc.GetRow();
          int col = loc.GetCol();

          if (color == RED || color == YELLOW) {
            eval += piece_square_table_[color][piece_type][row][col];
          } else {
            eval -= piece_square_table_[color][piece_type][row][col];
          }
        }
      }
    }

    if (options_.enable_piece_activation) {
      auto team_activation_score = [](int n_player1, int n_player2) {
        constexpr int A = 25;
        constexpr int B = 15;
        return A * (n_player1 + n_player2) + B * n_player1 * n_player2;
      };

      int* n_activated = thread_state.NActivated();
      eval += team_activation_score(n_activated[RED], n_activated[YELLOW])
            - team_activation_score(n_activated[BLUE], n_activated[GREEN]);
    }

//    // DEBUG: return early
//    return maximizing_player ? eval : -eval;

    // Mobility evaluation
    if (options_.enable_mobility_evaluation) {
      int* total_moves = thread_state.TotalMoves();
      eval += 5 * (total_moves[RED] + total_moves[YELLOW]
                   - total_moves[BLUE] - total_moves[GREEN]);
    }

    auto lazy_skip = [&](int margin) {
      if (!options_.enable_lazy_eval) {
        return false;
      }
      int re = maximizing_player ? eval : -eval; // returned eval
      return re + margin <= alpha || re >= beta + margin;
    };

    int piece_development_margin = 400;
    int king_safety_margin = 600;

    if (lazy_skip(piece_development_margin + king_safety_margin)) {
      num_lazy_eval_++;
      return maximizing_player ? eval : -eval;
    }

    if (options_.enable_piece_imbalance) {
      const auto& piece_list = board.GetPieceList();
      int n_major_red = GetNumMajorPieces(piece_list[RED]);
      int n_major_yellow = GetNumMajorPieces(piece_list[YELLOW]);
      int n_major_blue = GetNumMajorPieces(piece_list[BLUE]);
      int n_major_green = GetNumMajorPieces(piece_list[GREEN]);

      int diff_ry = std::abs(n_major_red - n_major_yellow);
      int diff_bg = std::abs(n_major_blue - n_major_green);

      eval += kPieceImbalanceTable[diff_ry] - kPieceImbalanceTable[diff_bg];
    }

    if (lazy_skip(king_safety_margin)) {
      num_lazy_eval_++;
      return maximizing_player ? eval : -eval;
    }

    // King safety evaluation (no lazy eval)
    if (options_.enable_king_safety) {
      for (int color = 0; color < 4; ++color) {
        int king_safety = 0;
        PlayerColor pl_cl = static_cast<PlayerColor>(color);
        Player player(pl_cl);
        Team team = player.GetTeam();
        Team other = OtherTeam(team);
        const auto king_location = board.GetKingLocation(pl_cl);
        if (king_location.Present()) {

          if (options_.enable_pawn_shield) {
            bool shield = HasShield(board, pl_cl, king_location);
            bool on_back_rank = OnBackRank(king_location);
            if (!shield) {
              king_safety -= 75;
            }
            if (!on_back_rank) {
              king_safety -= 50;
            }
            if (!shield && !on_back_rank) {
              king_safety -= 50;
            }
          }

          if (options_.enable_attacking_king_zone) {

            for (int delta_row = -1; delta_row <= 1; ++delta_row) {
              for (int delta_col = -1; delta_col <= 1; ++delta_col) {
                int row = king_location.GetRow() + delta_row;
                int col = king_location.GetCol() + delta_col;
                BoardLocation loc(row, col);
                if (!board.IsLegalLocation(loc) || OnBackRank(loc)) {
                  continue;
                }
                BoardLocation piece_location(row, col);

                PlacedPiece attackers[5];
                size_t num_attackers = board.GetAttackers2(attackers, 5, other, piece_location);

                if (num_attackers > 0) {
                  int value_of_attacks = 0;
                  int attacker_colors[4] = {0, 0, 0, 0};
                  for (size_t attacker_id = 0; attacker_id < num_attackers; attacker_id++) {
                    const auto& placed_piece = attackers[attacker_id];
                    const auto& piece = placed_piece.GetPiece();
                    int val = king_attacker_values_[piece.GetPieceType()];
                    value_of_attacks += val;
                    if (val > 0) {
                      attacker_colors[piece.GetColor()]++;
                    }
                  }
                  int num_attacker_colors = 0;
                  for (int i = 0; i < 4; i++) {
                    if (attacker_colors[i] > 0) {
                      num_attacker_colors++;
                    }
                  }
                  int attack_zone = value_of_attacks * king_attack_weight_[num_attackers] / 100;
                  if (num_attacker_colors > 1) {
                    attack_zone += 50;
                  }
                  king_safety -= attack_zone;
                }
              }
            }
          }

        }

        if (color == RED || color == YELLOW) {
          eval += king_safety;
        } else {
          eval -= king_safety;
        }

      }
    }

  }
  // w.r.t. maximizing team
  return maximizing_player ? eval : -eval;
}

void ThreadState::ResetHistoryHeuristic() {
  std::memset(history_heuristic, 0, (14*14*14*14) * sizeof(int) / sizeof(char));
}

void AlphaBetaPlayer::ResetMobilityScores(ThreadState& thread_state) {
  // reset pseudo-mobility scores
  if (options_.enable_mobility_evaluation || options_.enable_piece_activation) {
    for (int i = 0; i < 4; i++) {
      Player player(static_cast<PlayerColor>(i));
      UpdateMobilityEvaluation(thread_state, player);
    }
  }
}

std::optional<std::tuple<int, std::optional<Move>, int>>
AlphaBetaPlayer::MakeMove(
    Board& board,
    std::optional<std::chrono::milliseconds> time_limit,
    int max_depth) {
  SetCanceled(false);
  // Use Alpha-Beta search with iterative deepening
  std::optional<std::chrono::time_point<std::chrono::system_clock>> deadline;
  auto start = std::chrono::system_clock::now();
  if (time_limit.has_value()) {
    deadline = start + time_limit.value();
  }

  if (options_.max_search_depth.has_value()) {
    max_depth = std::min(max_depth, options_.max_search_depth.value());
  }

  int num_threads = 1;
  if (options_.enable_multithreading) {
    num_threads = options_.num_threads;
  }
  assert(num_threads >= 1);
  std::vector<ThreadState> thread_states;
  thread_states.reserve(num_threads);
  for (int i = 0; i < num_threads; i++) {
    auto pv_copy = pv_info_.Copy();
    thread_states.emplace_back(options_, board, *pv_copy);
    auto& thread_state = thread_states.back();
    ResetMobilityScores(thread_state);
    thread_state.ResetHistoryHeuristic();
  }

  std::mutex mutex;
  std::optional<std::tuple<int, std::optional<Move>, int>> res;

  std::vector<std::unique_ptr<std::thread>> threads;
  for (size_t i = 0; i < thread_states.size(); i++) {
    threads.push_back(std::make_unique<std::thread>([
      i, &thread_states, deadline, max_depth, this, &res, &mutex] {
      ThreadState& thread_state = thread_states[i];
      auto r = MakeMoveSingleThread(thread_state, deadline,
          max_depth);
      SetCanceled(true);
      std::lock_guard<std::mutex> lock(mutex);
      if (!res.has_value()) {
        res = r;
        pv_info_ = thread_state.GetPVInfo();
      }
    }));
  }

  for (auto& thread : threads) {
    thread->join();
  }

  SetCanceled(false);
  return res;
}

std::optional<std::tuple<int, std::optional<Move>, int>>
AlphaBetaPlayer::MakeMoveSingleThread(
    ThreadState& thread_state,
    std::optional<std::chrono::time_point<std::chrono::system_clock>> deadline,
    int max_depth) {
  Board& board = thread_state.GetBoard();
  PVInfo& pv_info = thread_state.GetPVInfo();

  int next_depth = std::min(1 + pv_info.GetDepth(), max_depth);
  std::optional<std::tuple<int, std::optional<Move>>> res;
  int alpha = -kMateValue;
  int beta = kMateValue;
  bool maximizing_player = board.TeamToPlay() == RED_YELLOW;
  int searched_depth = 0;
  Stack stack[kMaxPly + 10];
  Stack* ss = stack + 7;

  while (next_depth <= max_depth) {
    std::optional<std::tuple<int, std::optional<Move>>> move_and_value;

    move_and_value = Search(
        ss, Root, thread_state, 1, next_depth, alpha, beta, maximizing_player,
        0, deadline, pv_info);

    if (!move_and_value.has_value()) { // Hit deadline
      break;
    }
    res = move_and_value;
    searched_depth = next_depth;
    next_depth++;
    int evaluation = std::get<0>(move_and_value.value());
    if (std::abs(evaluation) == kMateValue) {
      break;  // Proven win/loss
    }
  }

  if (res.has_value()) {
    int eval = std::get<0>(res.value());
    if (!maximizing_player) {
      eval = -eval;
    }
    return std::make_tuple(eval, std::get<1>(res.value()), searched_depth);
  }

  return std::nullopt;
}

int PVInfo::GetDepth() const {
  if (best_move_.has_value()) {
    if (child_ == nullptr) {
      return 1;
    }
    return 1 + child_->GetDepth();
  }
  return 0;
}

void AlphaBetaPlayer::UpdateMobilityEvaluation(
    ThreadState& thread_state, Player player) {
  Board& board = thread_state.GetBoard();

  Move* moves = thread_state.GetNextMoveBufferPartition();
  Player curr_player = board.GetTurn();
  board.SetPlayer(player);
  size_t num_moves = board.GetPseudoLegalMoves2(
      moves, kBufferPartitionSize);
  int color = player.GetColor();
  thread_state.TotalMoves()[color] = num_moves;

  if (options_.enable_piece_activation) {
    auto piece_activated = [this](
        int color, PieceType piece_type,
        const BoardLocation& location, int n_moves) {
      if (piece_type == KNIGHT) {
        // activated so long as it's not on the back rank
        int row = location.GetRow();
        int col = location.GetCol();
        bool back_rank = (color == RED && row == 13)
                      || (color == YELLOW && row == 0)
                      || (color == BLUE && col == 0)
                      || (color == GREEN && col == 13);
        return !back_rank;
      }
      return n_moves >= piece_activation_threshold_[piece_type];
    };

    // note: this computation is dependent on the implementation of
    // Board::GetPseudoLegalMoves2, which adds all moves for a given
    // piece/location at a time.
    BoardLocation last_loc = BoardLocation::kNoLocation;
    PieceType last_piece_type = NO_PIECE;
    int n_pieces_activated = 0;
    int n_moves = 0;
    for (size_t move_id = 0; move_id < num_moves; move_id++) {
      const auto& move = moves[move_id];
      const auto& from = move.From();
      const auto& piece = board.GetPiece(from);
      PieceType piece_type = piece.GetPieceType();
      if (piece_type == QUEEN || piece_type == ROOK || piece_type == BISHOP
          || piece_type == KNIGHT) {
        if (from != last_loc) {
          if (piece_activated(color, last_piece_type, last_loc, n_moves)) {
            n_pieces_activated++;
          }
          last_loc = from;
          last_piece_type = piece_type;
          n_moves = 0;
        }
        n_moves++;
      }
    }
    if (piece_activated(color, last_piece_type, last_loc, n_moves)) {
      n_pieces_activated++;
    }
    thread_state.NActivated()[color] = n_pieces_activated;
  }

  board.SetPlayer(curr_player);
  thread_state.ReleaseMoveBufferPartition();
}

bool AlphaBetaPlayer::OnBackRank(
    const BoardLocation& loc) {
  return loc.GetRow() == 0 || loc.GetRow() == 13 || loc.GetCol() == 0
    || loc.GetCol() == 13;
}

bool AlphaBetaPlayer::HasShield(
    Board& board, PlayerColor color, const BoardLocation& king_loc) {
  int row = king_loc.GetRow();
  int col = king_loc.GetCol();

  auto ray_blocked = [&](int delta_row, int delta_col) {
    for (int i = 0; i < 2; i++) {
      BoardLocation loc(row + delta_row * (i + 1), col + delta_col * (i + 1));
      if (!board.IsLegalLocation(loc)) {
        return true;
      }
      const auto piece = board.GetPiece(loc);
      if (piece.Present()
          && piece.GetColor() == color) {
        return true;
      }
    }
    return false;
  };

  bool has_shield = true;
  switch (color) {
  case RED:
    return ray_blocked(-1, -1) && ray_blocked(-1, 0) && ray_blocked(-1, 1);
  case BLUE:
    return ray_blocked(-1, 1) && ray_blocked(0, 1) && ray_blocked(1, 1);
  case YELLOW:
    return ray_blocked(1, -1) && ray_blocked(1, 0) && ray_blocked(1, 1);
  case GREEN:
    return ray_blocked(-1, -1) && ray_blocked(0, -1) && ray_blocked(1, -1);
  default:
    abort();
  }
  return has_shield;
}

std::shared_ptr<PVInfo> PVInfo::Copy() const {
  std::shared_ptr<PVInfo> copy = std::make_shared<PVInfo>();
  if (best_move_.has_value()) {
    copy->SetBestMove(best_move_.value());
  }
  std::shared_ptr<PVInfo> child = child_;
  if (child != nullptr) {
    child = child->Copy();
  }
  copy->SetChild(child);
  return copy;
}

}  // namespace chess
