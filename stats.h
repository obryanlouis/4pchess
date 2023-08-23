#ifndef _STATS_H_
#define _STATS_H_

#include <array>
#include <type_traits>

namespace chess {

template <typename T, int Size, int... Sizes>
struct Stats : public std::array<Stats<T, Sizes...>, Size>
{
  using stats = Stats<T, Size, Sizes...>;

  void Fill(const T& v) {
    assert(std::is_standard_layout<stats>::value);
    T* p = reinterpret_cast<T*>(this);
    std::fill(p, p + sizeof(*this) / sizeof(T), v);
  }
};


template <typename T, int Size>
struct Stats<T, Size> : public std::array<T, Size> {};

}  // namespace chess

#endif  // _STATS_H_
