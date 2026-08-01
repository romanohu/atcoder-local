#ifndef ATCODER_LOCAL_CORE_HPP
#define ATCODER_LOCAL_CORE_HPP

#include <algorithm>

using ll = long long;
using ull = unsigned long long;

#define rep(i, n) for (int i = 0; i < static_cast<int>(n); ++i)

template <class T, class U>
bool chmin(T& current, const U& candidate) {
    if (candidate < current) {
        current = candidate;
        return true;
    }
    return false;
}

template <class T, class U>
bool chmax(T& current, const U& candidate) {
    if (current < candidate) {
        current = candidate;
        return true;
    }
    return false;
}

#endif
