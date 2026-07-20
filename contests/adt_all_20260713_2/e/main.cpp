//#include <bits/stdc++.h>
#include <algorithm>
#include <iostream>
#include <vector>
using namespace std;

#define rep(i, N) for (int i = 0; i < (N); ++i)

int solve(const vector<int>& H) {
    int ans = 0;
    int height = 0;
    int current = 0;
    int size = (int)H.size();

    for (int i = 0; i < size; ++i) {
        if (height != H[i]) {
            current = 0;
            height = H[i];
        }

        ++current;
        ans = max(ans, current);
    }

    return ans;
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int N;
    cin >> N;

    vector<int> H(N);
    rep(i, N) {
        cin >> H[i];
    }

    int ans = 1;

    for (int d = 1; d < N; ++d) {
        for (int start = 0; start < d; ++start) {
            vector<int> buildings;

            for (int i = start; i < N; i += d) {
                buildings.push_back(H[i]);
            }

            ans = max(ans, solve(buildings));
        }
    }

    cout << ans << '\n';

    return 0;
}