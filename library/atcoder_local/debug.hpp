#ifndef ATCODER_LOCAL_DEBUG_HPP
#define ATCODER_LOCAL_DEBUG_HPP

#include <iostream>

namespace atcoder_local {

inline void debug_log() {
    std::cerr << '\n';
}

template <class T, class... Ts>
void debug_log(const T& first, const Ts&... rest) {
    std::cerr << first;
    ((std::cerr << ' ' << rest), ...);
    std::cerr << '\n';
}

}  // namespace atcoder_local

#ifdef LOCAL
#define DBG(...) atcoder_local::debug_log(__VA_ARGS__)
#else
#define DBG(...) ((void)0)
#endif

#endif
