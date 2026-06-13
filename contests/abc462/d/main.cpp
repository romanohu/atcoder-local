//#include <bits/stdc++.h>
#include <iostream>
#include <algorithm>
#include <vector>
using namespace std;
using ll = long long;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int N, D;
    cin >> N >> D;

    vector<pair<int, int>> hitobito(N);

    int time_max = 0;

    for (int i = 0; i < N; ++i){
        int L, R;
        cin >> L >> R;
        hitobito[i] = {L, R};
        time_max = max(time_max, R);
    }

    vector<ll> diff(time_max + 3, 0);

    for (auto p : hitobito){
        int L = p.first;
        int R = p.second;

        if (R - L + 1 < D) 
            continue;
        diff[L] += 1;
        diff[R - D + 2] -= 1;
    }

    ll ans = 0;
    ll cnt = 0;

    for (int t = 0; t <= time_max; ++t){
        cnt += diff[t];
        ans += cnt * (cnt - 1) / 2;
    }

    cout << ans << '\n';

    return 0;
}