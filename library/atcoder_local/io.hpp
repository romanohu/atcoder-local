#ifndef ATCODER_LOCAL_IO_HPP
#define ATCODER_LOCAL_IO_HPP

#include <iostream>

namespace atcoder_local {

inline void setup_io() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
}

template <class... Ts>
void read(Ts&... values) {
    ((std::cin >> values), ...);
}

inline void print() {
    std::cout << '\n';
}

template <class T, class... Ts>
void print(const T& first, const Ts&... rest) {
    std::cout << first;
    ((std::cout << ' ' << rest), ...);
    std::cout << '\n';
}

}  // namespace atcoder_local

#endif
