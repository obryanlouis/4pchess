// addon.cc
#include <node.h>
//#include "myobject.h"
#include "board_wrapper.h"

namespace demo {

using v8::Context;
using v8::Local;
using v8::Object;
using v8::Value;

//void InitAll(Local<Object> exports) {
//  board_wrapper::PlacedPiece::Init(exports);
//  board_wrapper::CastlingRights::Init(exports);
//  board_wrapper::Board::Init(exports);
//  board_wrapper::Player::Init(exports);
//}
//
//NODE_MODULE(NODE_GYP_MODULE_NAME, InitAll)


extern "C" NODE_MODULE_EXPORT void
NODE_MODULE_INITIALIZER(Local<Object> exports,
                        Local<Value> module,
                        Local<Context> context) {
  /* Perform addon initialization steps here. */
  board_wrapper::PlacedPiece::Init(exports);
  board_wrapper::CastlingRights::Init(exports);
  board_wrapper::Board::Init(exports);
  board_wrapper::Player::Init(exports);
}

}  // namespace demo
