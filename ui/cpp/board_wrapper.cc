#include "board_wrapper.h"

#include <memory>
#include <mutex>
#include <chrono>
#include <cmath>
#include <string>
#include <unordered_map>
#include <vector>
#include <sstream>
#include <iostream>

namespace board_wrapper {

using chess::BoardLocation;
using chess::Move;
using chess::Piece;
using chess::PieceType;
using chess::PlayerColor;
using v8::Array;
using v8::Context;
using v8::External;
using v8::Function;
using v8::FunctionCallbackInfo;
using v8::FunctionTemplate;
using v8::Isolate;
using v8::Local;
using v8::MaybeLocal;
using v8::Number;
using v8::Object;
using v8::ObjectTemplate;
using v8::String;
using v8::Value;


std::mutex Player::mutex_;
std::vector<Player*> Player::players_;
std::shared_ptr<chess::AlphaBetaPlayer> Player::last_player_;
int64_t Player::last_board_hash_;


PlacedPiece::PlacedPiece(v8::Isolate* isolate, int row, int col, int piece_type, int player_color)
  : location_(row, col) {
  assert(0 <= player_color && player_color < 4);
  piece_ = Piece(chess::Player(static_cast<PlayerColor>(player_color)),
                 static_cast<PieceType>(piece_type));
  // Register to delete the object.
  node::AddEnvironmentCleanupHook(isolate, PlacedPiece::DeleteInstance, this);
}

void PlacedPiece::Init(v8::Local<v8::Object> exports) {
  Isolate* isolate = exports->GetIsolate();
  Local<Context> context = isolate->GetCurrentContext();

  Local<ObjectTemplate> addon_data_tpl = ObjectTemplate::New(isolate);
  addon_data_tpl->SetInternalFieldCount(1);  // 1 field for PlacedPiece::New
  Local<Object> addon_data =
      addon_data_tpl->NewInstance(context).ToLocalChecked();

  // Prepare constructor template
  Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New, addon_data);
  tpl->SetClassName(String::NewFromUtf8(isolate, "PlacedPiece").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);  // 1 object pointer

  // Prototype
  NODE_SET_PROTOTYPE_METHOD(tpl, "debugString", PlacedPiece::DebugString);

  Local<Function> constructor = tpl->GetFunction(context).ToLocalChecked();
  addon_data->SetInternalField(0, constructor);
  exports->Set(context, String::NewFromUtf8(
      isolate, "PlacedPiece").ToLocalChecked(),
      constructor).FromJust();
}

void PlacedPiece::New(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = args.GetIsolate();
  Local<Context> context = isolate->GetCurrentContext();

  if (args.IsConstructCall()) {
    // Invoked as constructor: `new PlacedPiece(...)`
    int row = args[0]->Int32Value(context).FromJust();
    int col = args[1]->Int32Value(context).FromJust();
    int piece_type = args[2]->Int32Value(context).FromJust();
    int player_color = args[3]->Int32Value(context).FromJust();

    PlacedPiece* obj = new PlacedPiece(isolate, row, col, piece_type, player_color);
    obj->Wrap(args.This());
    args.GetReturnValue().Set(args.This());
  } else {
    // Invoked as plain function `PlacedPiece(...)`, turn into construct call.
    const int argc = 4;
    Local<Value> argv[argc] = { args[0], args[1], args[2], args[3] };
    Local<Function> cons =
        args.Data().As<Object>()->GetInternalField(0).As<Function>();
    Local<Object> result =
        cons->NewInstance(context, argc, argv).ToLocalChecked();
    args.GetReturnValue().Set(result);
  }
}

std::string PlacedPiece::ToStr() const {
  return "PlacedPiece(Loc("
    + std::to_string(location_.GetRow())
    + ", "
    + std::to_string(location_.GetCol())
    + "), "
    + std::to_string(static_cast<int>(piece_.GetPieceType()))
    + ", "
    + std::to_string(static_cast<int>(piece_.GetColor()))
    + ")";
}

void PlacedPiece::DebugString(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = args.GetIsolate();

  PlacedPiece* obj = MyObjectWrap::Unwrap<PlacedPiece>(args.Holder());

  std::string s = obj->ToStr();
  MaybeLocal<String> local_s = String::NewFromUtf8(isolate, s.data());
  args.GetReturnValue().Set(local_s.ToLocalChecked());
}


CastlingRights::CastlingRights(v8::Isolate* isolate, int player_color, bool kingside, bool queenside)
  : player_(static_cast<PlayerColor>(player_color)),
    rights_(kingside, queenside) {
  assert(0 <= player_color && player_color < 4);
  // Register to delete the object.
  node::AddEnvironmentCleanupHook(isolate, CastlingRights::DeleteInstance, this);
}

void CastlingRights::Init(v8::Local<v8::Object> exports) {
  Isolate* isolate = exports->GetIsolate();
  Local<Context> context = isolate->GetCurrentContext();

  Local<ObjectTemplate> addon_data_tpl = ObjectTemplate::New(isolate);
  addon_data_tpl->SetInternalFieldCount(1);  // 1 field for CastlingRights::New
  Local<Object> addon_data =
      addon_data_tpl->NewInstance(context).ToLocalChecked();

  // Prepare constructor template
  Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New, addon_data);
  tpl->SetClassName(String::NewFromUtf8(isolate, "CastlingRights").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);  // 1 object pointer

  // Prototype
  NODE_SET_PROTOTYPE_METHOD(tpl, "debugString", CastlingRights::DebugString);

  Local<Function> constructor = tpl->GetFunction(context).ToLocalChecked();
  addon_data->SetInternalField(0, constructor);
  exports->Set(context, String::NewFromUtf8(
      isolate, "CastlingRights").ToLocalChecked(),
      constructor).FromJust();
}

void CastlingRights::New(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = args.GetIsolate();
  Local<Context> context = isolate->GetCurrentContext();

  if (args.IsConstructCall()) {
    // Invoked as constructor: `new CastlingRights(...)`
    int player_color = args[0]->Int32Value(context).FromJust();
    bool kingside = args[1]->BooleanValue(isolate);
    bool queenside = args[2]->BooleanValue(isolate);

    CastlingRights* obj = new CastlingRights(isolate, player_color, kingside, queenside);
    obj->Wrap(args.This());
    args.GetReturnValue().Set(args.This());
  } else {
    // Invoked as plain function `CastlingRights(...)`, turn into construct call.
    const int argc = 3;
    Local<Value> argv[argc] = { args[0], args[1], args[2] };
    Local<Function> cons =
        args.Data().As<Object>()->GetInternalField(0).As<Function>();
    Local<Object> result =
        cons->NewInstance(context, argc, argv).ToLocalChecked();
    args.GetReturnValue().Set(result);
  }
}

std::string CastlingRights::ToStr() const {
  return "CastlingRights("
    + std::to_string(player_.GetColor())
    + ", "
    + std::to_string(rights_.Kingside())
    + ", "
    + std::to_string(rights_.Queenside())
    + ")";
}

void CastlingRights::DebugString(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = args.GetIsolate();

  CastlingRights* obj = MyObjectWrap::Unwrap<CastlingRights>(args.Holder());

  std::string s = obj->ToStr();
  MaybeLocal<String> local_s = String::NewFromUtf8(isolate, s.data());
  args.GetReturnValue().Set(local_s.ToLocalChecked());
}

Board::Board(v8::Isolate* isolate, std::unique_ptr<chess::Board> board)
  : board_(std::move(board)) {
  // Register to delete the object.
  node::AddEnvironmentCleanupHook(isolate, Board::DeleteInstance, this);
}

void Board::Init(v8::Local<v8::Object> exports) {
  Isolate* isolate = exports->GetIsolate();
  Local<Context> context = isolate->GetCurrentContext();

  Local<ObjectTemplate> addon_data_tpl = ObjectTemplate::New(isolate);
  addon_data_tpl->SetInternalFieldCount(1);  // 1 field for Board::New
  Local<Object> addon_data =
      addon_data_tpl->NewInstance(context).ToLocalChecked();

  // Prepare constructor template
  Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, Board::New, addon_data);
  tpl->SetClassName(String::NewFromUtf8(isolate, "Board").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);  // 1 object pointer

  // Prototype
  NODE_SET_PROTOTYPE_METHOD(tpl, "debugString", Board::DebugString);

  Local<Function> constructor = tpl->GetFunction(context).ToLocalChecked();
  addon_data->SetInternalField(0, constructor);
  exports->Set(context, String::NewFromUtf8(
      isolate, "Board").ToLocalChecked(),
      constructor).FromJust();
}

void Board::New(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = args.GetIsolate();
  Local<Context> context = isolate->GetCurrentContext();

  if (args.IsConstructCall()) {
    std::unordered_map<chess::Player, chess::CastlingRights> castling_map;
    std::unordered_map<BoardLocation, Piece> piece_map;

    // Invoked as constructor: `new Board(...)`
    int turn = args[0]->Int32Value(context).FromJust();
    assert(0 <= turn && turn < 4);

    assert(args[1]->IsArray());
    v8::Array* pieces_array = v8::Array::Cast(*args[1]);
    for (size_t i = 0; i < pieces_array->Length(); ++i) {
      MaybeLocal<Value> piece = pieces_array->Get(context, i);
      PlacedPiece* placed_piece = MyObjectWrap::Unwrap<PlacedPiece>(
          piece.ToLocalChecked()->ToObject(context).ToLocalChecked());
      piece_map[placed_piece->GetLocation()] = placed_piece->GetPiece();
    }
    v8::Array* castling_array = v8::Array::Cast(*args[2]);
    for (size_t i = 0; i < castling_array->Length(); ++i) {
      MaybeLocal<Value> rights = castling_array->Get(context, i);
      CastlingRights* castling_rights = MyObjectWrap::Unwrap<CastlingRights>(
          rights.ToLocalChecked()->ToObject(context).ToLocalChecked());
      castling_map[castling_rights->GetPlayer()] = castling_rights->GetRights();
    }

    auto board = std::make_unique<chess::Board>(
        chess::Player(static_cast<PlayerColor>(turn)),
        std::move(piece_map),
        std::move(castling_map));

    Board* obj = new Board(isolate, std::move(board));
    obj->Wrap(args.This());
    args.GetReturnValue().Set(args.This());
  } else {
    // Invoked as plain function `Board(...)`, turn into construct call.
    const int argc = 3;
    Local<Value> argv[argc] = { args[0], args[1], args[2] };
    Local<Function> cons =
        args.Data().As<Object>()->GetInternalField(0).As<Function>();
    Local<Object> result =
        cons->NewInstance(context, argc, argv).ToLocalChecked();
    args.GetReturnValue().Set(result);
  }
}

std::string Board::ToStr() const {
  std::stringstream ss;
  ss << *board_;
  return ss.str();
}

void Board::DebugString(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = args.GetIsolate();

  Board* obj = MyObjectWrap::Unwrap<Board>(args.Holder());

  std::string s = obj->ToStr();
  MaybeLocal<String> local_s = String::NewFromUtf8(isolate, s.data());
  args.GetReturnValue().Set(local_s.ToLocalChecked());
}

void Player::Init(v8::Local<v8::Object> exports) {
  Isolate* isolate = exports->GetIsolate();
  Local<Context> context = isolate->GetCurrentContext();

  Local<ObjectTemplate> addon_data_tpl = ObjectTemplate::New(isolate);
  addon_data_tpl->SetInternalFieldCount(1);  // 1 field for Player::New
  Local<Object> addon_data =
      addon_data_tpl->NewInstance(context).ToLocalChecked();

  // Prepare constructor template
  Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, Player::New, addon_data);
  tpl->SetClassName(String::NewFromUtf8(isolate, "Player").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);  // 1 object pointer

  // Prototype
  NODE_SET_PROTOTYPE_METHOD(tpl, "makeMove", Player::MakeMove);
  NODE_SET_PROTOTYPE_METHOD(tpl, "cancelEvaluation", Player::CancelEvaluation);

  Local<Function> constructor = tpl->GetFunction(context).ToLocalChecked();
  addon_data->SetInternalField(0, constructor);
  exports->Set(context, String::NewFromUtf8(
      isolate, "Player").ToLocalChecked(),
      constructor).FromJust();
}

void Player::New(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = args.GetIsolate();
  Local<Context> context = isolate->GetCurrentContext();

  if (args.IsConstructCall()) {
    Player* obj = new Player(isolate);
    obj->Wrap(args.This());
    args.GetReturnValue().Set(args.This());
  } else {
    // Invoked as plain function `Player(...)`, turn into construct call.
    const int argc = 0;
    Local<Value> *argv = nullptr;
    Local<Function> cons =
        args.Data().As<Object>()->GetInternalField(0).As<Function>();
    Local<Object> result =
        cons->NewInstance(context, argc, argv).ToLocalChecked();
    args.GetReturnValue().Set(result);
  }
}

void Player::MakeMove(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = args.GetIsolate();
  Local<Context> context = isolate->GetCurrentContext();

  Player* player_wrap = MyObjectWrap::Unwrap<Player>(args.Holder());
  Board* board_wrap = MyObjectWrap::Unwrap<Board>(
      args[0]->ToObject(context).ToLocalChecked());
  chess::Board* board = board_wrap->GetBoard();
  int64_t board_hash = board->HashKey();

  std::shared_ptr<chess::AlphaBetaPlayer> player_ptr = player_wrap->GetPlayer();
  if (player_ptr == nullptr) {
    player_ptr = GetLatestPlayer(board_hash);
    if (player_ptr == nullptr) {
      player_ptr = std::make_shared<chess::AlphaBetaPlayer>();
    }
    player_wrap->SetPlayer(player_ptr);
  }

  chess::AlphaBetaPlayer& player = *player_ptr;
  SetLatestPlayer(player_ptr, board_hash);
  player_ptr->SetCanceled(false);

  int depth = args[1]->Int32Value(context).FromJust();
  // by default, there's no time limit
  std::optional<std::chrono::milliseconds> time_limit;
  auto secs = args[2]->Int32Value(context);
  if (args[1]->IsInt32() && !secs.IsNothing() && secs.FromJust() > 0) {
    time_limit = std::chrono::milliseconds(1000 * secs.FromJust());
  }
  //std::chrono::milliseconds time_limit(10'000);
  auto move_res = player.MakeMove(*board, time_limit, depth);

  if (move_res.has_value()) {
    // Return format:
    //  {'evaluation': float,
    //   'principal_variation': [
    //    {'turn': int,
    //     'from': {'row': int, 'col': int},
    //     'to': {'row': int, 'col': int}}
    //   ]}

    float evaluation = std::get<0>(move_res.value()) / 100.0;

    Local<Object> res = Object::New(isolate);

    if (std::isinf(evaluation)) {
      if (evaluation > 0) {
        // +inf
        res->Set(context, String::NewFromUtf8Literal(isolate, "evaluation"),
                 String::NewFromUtf8Literal(isolate, "Infinity")).Check();
      } else {
        // -inf
        res->Set(context, String::NewFromUtf8Literal(isolate, "evaluation"),
                 String::NewFromUtf8Literal(isolate, "-Infinity")).Check();
      }
    } else {
      res->Set(context, String::NewFromUtf8Literal(isolate, "evaluation"),
               v8::Number::New(isolate, evaluation)).Check();
    }

    const chess::PVInfo* pv_info = &player.GetPVInfo();
    std::vector<Move> pv_moves;
    pv_moves.reserve(4);
    int num_moves = 0;
    while (pv_info != nullptr && num_moves < 4) {
      const auto& best_move = pv_info->GetBestMove();
      if (best_move.has_value()) {
        num_moves++;
        pv_moves.push_back(best_move.value());
      }
      pv_info = pv_info->GetChild().get();
    }
    int search_depth = std::get<2>(move_res.value());
    res->Set(context, String::NewFromUtf8Literal(isolate, "search_depth"),
             v8::Integer::New(isolate, search_depth)).Check();

    Local<Array> principal_variation = Array::New(isolate, pv_moves.size());
    chess::Player player = board->GetTurn();
    for (size_t i = 0; i < pv_moves.size(); ++i) {
      const auto& move = pv_moves[i];
      Local<Object> pv = Object::New(isolate);

      // Set "turn"
      pv->Set(context, String::NewFromUtf8Literal(isolate, "turn"),
              v8::Integer::New(isolate, static_cast<int>(player.GetColor())))
        .Check();

      // Set "from"
      const auto& loc_from = move.From();
      Local<Object> from = Object::New(isolate);
      from->Set(context, String::NewFromUtf8Literal(isolate, "row"),
              v8::Integer::New(isolate, loc_from.GetRow())).Check();
      from->Set(context, String::NewFromUtf8Literal(isolate, "col"),
              v8::Integer::New(isolate, loc_from.GetCol())).Check();
      pv->Set(context, String::NewFromUtf8Literal(isolate, "from"), from)
        .Check();

      // Set "to"
      const auto& loc_to = move.To();
      Local<Object> to = Object::New(isolate);
      to->Set(context, String::NewFromUtf8Literal(isolate, "row"),
              v8::Integer::New(isolate, loc_to.GetRow())).Check();
      to->Set(context, String::NewFromUtf8Literal(isolate, "col"),
              v8::Integer::New(isolate, loc_to.GetCol())).Check();
      pv->Set(context, String::NewFromUtf8Literal(isolate, "to"), to).Check();

      // Add to array
      principal_variation->Set(context, i, pv).Check();

      player = chess::GetNextPlayer(player);
    }

    res->Set(
        context, String::NewFromUtf8Literal(isolate, "principal_variation"),
        principal_variation).Check();

    args.GetReturnValue().Set(res);
  }

}

void Player::CancelEvaluation(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Player* obj = MyObjectWrap::Unwrap<Player>(args.Holder());
  auto player = obj->GetPlayer();
  if (player != nullptr) {
    player->CancelEvaluation();
  }
}

Player::Player(v8::Isolate* isolate) {
  CancelAllEvaluations();
  AddToGlobalObjList(this);
  // Register to delete the object.
  node::AddEnvironmentCleanupHook(isolate, Player::DeleteInstance, this);
}

void Player::AddToGlobalObjList(Player* obj) {
  std::lock_guard lock(mutex_);
  players_.push_back(obj);
}

void Player::RemoveFromGlobalObjList(Player* obj) {
  std::lock_guard lock(mutex_);
  auto it = std::find(players_.begin(), players_.end(), obj);
  if (it != players_.end()) {
    players_.erase(it);
  }
}

void Player::CancelAllEvaluations() {
  std::lock_guard lock(mutex_);
  for (Player* obj : players_) {
    auto player = obj->GetPlayer();
    if (player != nullptr) {
      player->CancelEvaluation();
    }
  }
}


void Player::SetLatestPlayer(std::shared_ptr<chess::AlphaBetaPlayer> player,
    int64_t board_hash) {
  std::lock_guard lock(mutex_);
  if (last_player_ == nullptr || last_board_hash_ != board_hash) {
    last_player_ = player;
    last_board_hash_ = board_hash;
  }
}

std::shared_ptr<chess::AlphaBetaPlayer> Player::GetLatestPlayer(
    int64_t board_hash) {
  std::lock_guard lock(mutex_);
  if (last_board_hash_ == board_hash) {
    return last_player_;
  }
  return nullptr;
}


}  // namespace board_wrapper
