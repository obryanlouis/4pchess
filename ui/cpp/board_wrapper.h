// board_wrapper.h
#ifndef _BOARD_WRAPPER_H_
#define _BOARD_WRAPPER_H_

#include <string>
#include <vector>
#include <mutex>

#include <node.h>
#include <node_object_wrap.h>
#include "../../board.h"
#include "../../player.h"

namespace board_wrapper {

class PlacedPiece : public node::ObjectWrap {
 public:
  PlacedPiece(const PlacedPiece&) = default;
  static void Init(v8::Local<v8::Object> exports);
  static void DeleteInstance(void* data) { delete static_cast<PlacedPiece*>(data); }

  const chess::BoardLocation GetLocation() const { return location_; }
  const chess::Piece GetPiece() const { return piece_; }

 private:
  PlacedPiece(v8::Isolate* isolate, int row, int col, int piece_type, int player_color);
  std::string ToStr() const;

  static void New(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void DebugString(const v8::FunctionCallbackInfo<v8::Value>& args);

  chess::BoardLocation location_;
  chess::Piece piece_;
};

class CastlingRights : public node::ObjectWrap {
 public:
  static void Init(v8::Local<v8::Object> exports);
  static void DeleteInstance(void* data) { delete static_cast<CastlingRights*>(data); }

  const chess::Player& GetPlayer() const { return player_; }
  const chess::CastlingRights& GetRights() const { return rights_; }

 private:
  CastlingRights(v8::Isolate* isolate, int player_color, bool kingside, bool queenside);
  std::string ToStr() const;

  static void New(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void DebugString(const v8::FunctionCallbackInfo<v8::Value>& args);

  chess::Player player_;
  chess::CastlingRights rights_;
};

class Board : public node::ObjectWrap {
 public:
  static void Init(v8::Local<v8::Object> exports);
  static void DeleteInstance(void* data) { delete static_cast<Board*>(data); }

  chess::Board* GetBoard() { return board_.get(); }

 private:
  explicit Board(v8::Isolate* isolate, std::unique_ptr<chess::Board> board);
  std::string ToStr() const;

  static void New(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void DebugString(const v8::FunctionCallbackInfo<v8::Value>& args);

  std::unique_ptr<chess::Board> board_;
};

class Player : public node::ObjectWrap {
 public:
  Player(v8::Isolate* isolate);
  ~Player();

  static void Init(v8::Local<v8::Object> exports);
  static void DeleteInstance(void* data) { delete static_cast<Player*>(data); }

  chess::AlphaBetaPlayer& GetPlayer() { return player_; }

 private:
  static void New(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void MakeMove(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void CancelEvaluation(const v8::FunctionCallbackInfo<v8::Value>& args);

  // nuclear option: kill all other requests when a new obj is created
  static std::mutex mutex_;
  static std::vector<Player*> players_;
  static void AddToGlobalObjList(Player* obj);
  static void RemoveFromGlobalObjList(Player* obj);
  static void CancelAllEvaluations();

  chess::AlphaBetaPlayer player_;
};

}  // namespace board_wrapper

#endif  // _BOARD_WRAPPER_H_
