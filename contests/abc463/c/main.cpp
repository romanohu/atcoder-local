//#include <bits/stdc++.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <functional>
using namespace std;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int N;
    cin >> N;

    vector<pair<int, int>> takahasis(N);
    for (int i = 0; i < N; ++i)
        cin >> takahasis[i].first >> takahasis[i].second;

    sort(takahasis.begin(), takahasis.end(), greater<pair<int, int>>());

    vector<int> fix_takahasis(N);
    fix_takahasis[0] = takahasis[0].second;
    for (int i = 1; i < N; ++i) {
        fix_takahasis[i] = max(fix_takahasis[i - 1], takahasis[i].second);
    }

    int Q;
    cin >> Q;

    for (int i = 0; i < Q; ++i) {
        int q;
        cin >> q;

        auto it = upper_bound(fix_takahasis.begin(), fix_takahasis.end(), q);

        int idx = it - fix_takahasis.begin();
        cout << takahasis[idx].first << '\n';
    }

    return 0;
}