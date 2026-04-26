//#include <bits/stdc++.h>
#include <iostream>
#include <queue>
#include <vector>
#include <set>
#include <map>
using namespace std;
using ll = long long;
using P = std::pair<ll,ll>;


int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int N, K;
    cin >> N >> K;

    set<P> st;
    map<ll,ll> cnt;

    ll total = 0;

    for (int i=0; i<N; ++i){
        int enter;
        cin >> enter;

        if (cnt.count(enter)) {
            st.erase({cnt[enter], enter});
            total -= cnt[enter];
        }
        cnt[enter] += enter;
        st.insert({cnt[enter], enter});
        total += cnt[enter];
    }

    while (!st.empty() && K--) {
        auto it = prev(st.end());

        total -= it->first;
        cnt.erase(it->second);
        st.erase(it);
    }

    cout << total << "\n";

    return 0;
}
