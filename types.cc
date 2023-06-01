#include <cmath>
#include <cstdlib>

#include "types.h"

namespace chess {

std::string BoardLocation::PrettyStr() const {
  std::string s;
  s += ('a' + GetCol());
  s += std::to_string(14 - GetRow());
  return s;
}

std::ostream& operator<<(
    std::ostream& os, const BoardLocation& location) {
  os << "Loc(" << (int)location.GetRow() << ", " << (int)location.GetCol() << ")";
  return os;
}

}  // namespace chess

