// board_wrapper.h
#ifndef _BOARD_WRAPPER_H_
#define _BOARD_WRAPPER_H_

#include <string>
#include <vector>
#include <mutex>
#include <memory>

#include <iostream>
#include <node.h>
#include <node_object_wrap.h>
#include "../../board.h"
#include "../../player.h"

namespace board_wrapper {

// Like node::ObjectWrap, except it doesn't delete the itself automatically.
// Individual subclasses are responsible for deleting instances.
// node::ObjectWrap sets a weak callback that can result in double-deleting
// instances, causing segfaults.
class MyObjectWrap {
 public:

  template <class T>
  static inline T* Unwrap(v8::Local<v8::Object> handle) {
    assert(!handle.IsEmpty());
    assert(handle->InternalFieldCount() > 0);
    void* ptr = handle->GetAlignedPointerFromInternalField(0);
    MyObjectWrap* wrap = static_cast<MyObjectWrap*>(ptr);
    return static_cast<T*>(wrap);
  }

  inline void Wrap(v8::Local<v8::Object> handle) {
    assert(handle_.IsEmpty());
    assert(handle->InternalFieldCount() > 0);
    handle->SetAlignedPointerInInternalField(0, this);
    handle_.Reset(v8::Isolate::GetCurrent(), handle);
  }

 private:
  v8::Persistent<v8::Object> handle_;
};

class PlacedPiece : public MyObjectWrap {
 public:
  PlacedPiece(const PlacedPiece&) = default;
  static void Init(v8::Local<v8::Object> exports);
  static void DeleteInstance(void* data) {
    delete static_cast<PlacedPiece*>(data);
  }

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

class CastlingRights : public MyObjectWrap {
 public:
  static void Init(v8::Local<v8::Object> exports);
  static void DeleteInstance(void* data) {
    delete static_cast<CastlingRights*>(data);
  }

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

class Board : public MyObjectWrap {
 public:
  static void Init(v8::Local<v8::Object> exports);
  static void DeleteInstance(void* data) {
    delete static_cast<Board*>(data);
  }

  chess::Board* GetBoard() { return board_.get(); }

 private:
  explicit Board(v8::Isolate* isolate, std::unique_ptr<chess::Board> board);
  std::string ToStr() const;

  static void New(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void DebugString(const v8::FunctionCallbackInfo<v8::Value>& args);

  std::unique_ptr<chess::Board> board_;
};

class Player : public MyObjectWrap {
 public:
  Player(v8::Isolate* isolate);
  ~Player() {
    RemoveFromGlobalObjList(this);
  }

  static void Init(v8::Local<v8::Object> exports);
  static void DeleteInstance(void* data) {
    Player* p = static_cast<Player*>(data);
    RemoveFromGlobalObjList(p);
    delete p;
  }

  std::shared_ptr<chess::AlphaBetaPlayer> GetPlayer() {
    return player_;
  }
  void SetPlayer(std::shared_ptr<chess::AlphaBetaPlayer> player) {
    player_ = player;
  }

 private:
  static void New(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void MakeMove(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void CancelEvaluation(const v8::FunctionCallbackInfo<v8::Value>& args);

  // nuclear option: kill all other requests when a new obj is created
  static std::mutex mutex_;
  static std::vector<Player*> players_;

  // keep track of the last player & its board to reuse it and save time on
  // consecutive searches.
  static std::shared_ptr<chess::AlphaBetaPlayer> last_player_;
  static std::shared_ptr<chess::NNUE> nnue_;
  static int64_t last_board_hash_;

  static void AddToGlobalObjList(Player* obj);
  static void RemoveFromGlobalObjList(Player* obj);
  static void CancelAllEvaluations();
  static void SetLatestPlayer(std::shared_ptr<chess::AlphaBetaPlayer> player,
                              int64_t board_hash);
  static std::shared_ptr<chess::AlphaBetaPlayer> GetLatestPlayer(
      int64_t board_hash);
  static std::shared_ptr<chess::NNUE> GetNNUE();

  std::shared_ptr<chess::AlphaBetaPlayer> player_;
};

}  // namespace board_wrapper

#endif  // _BOARD_WRAPPER_H_
