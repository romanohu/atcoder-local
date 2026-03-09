//#include <bits/stdc++.h>
#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;
using ll = long long;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int N, Q;
    cin >> N >> Q;

    vector<ll> A(N + 1);
    for (int i = 1; i <= N; i++) cin >> A[i];

    vector<pair<ll, int>> ord;
    ord.reserve(N);
    for (int i = 1; i <= N; i++) {
        ord.push_back({A[i], i});
    }
    sort(ord.begin(), ord.end());

    vector<int> used(N + 1, 0);
    int timer = 1;

    while (Q--) {
        int K;
        cin >> K;

        for (int i = 0; i < K; i++) {
            int b;
            cin >> b;
            used[b] = timer;
        }

        ll ans = -1;
        for (int i = 0; i <= K; i++) {
            int idx = ord[i].second;
            if (used[idx] != timer) {
                ans = ord[i].first;
                break;
            }
        }

        cout << ans << '\n';
        timer++;
    }

    return 0;
}