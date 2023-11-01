#ifndef _STATIC_EXCHANGE_H_
#define _STATIC_EXCHANGE_H_

#include <optional>
#include <tuple>

#include "board.h"


namespace chess {

// Returns the static exchange evaluation of a capture.
int StaticExchangeEvaluationCapture(
    int piece_evaluations[6],
    Board& board,
    const Move& move);


}  // namespace chess

#endif  // _STATIC_EXCHANGE_H_

