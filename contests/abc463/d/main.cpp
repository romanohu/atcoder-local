//#include <bits/stdc++.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <functional>
using namespace std;
using ll = long long;

int judge(pair<int, int> A, pair<int, int> B) {
    if (A.first > B.first)
        swap(A, B);

    if (A.second > B.first)
        return 0;

    return B.first - A.second;
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int N, K;
    cin >> N >> K;

    vector<pair<int, int>> nunos(N);
    for (int i = 0; i < N; ++i)
        cin >> nunos[i].first >> nunos[i].second;

    vector<vector<int>> scores(N);
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            if (i == j)
                continue;

            int score = judge(nunos[i], nunos[j]);
            if (score != 0) {
                scores[i].push_back(score);
            }
        }
    }

    int max_size = 0;
    for (int i = 0; i < N; ++i) {
        int score_size = scores[i].size();
        if (score_size > max_size) {
            max_size = score_size;
        }

        sort(scores[i].begin(), scores[i].end(), greater<int>());
    }

    if (max_size < K) {
        cout << -1 << "\n";
        return 0;
    }

    int ans = 0;
    for (int i = 0; i < N; ++i) {
        int score_size = scores[i].size();
        if (score_size < K)
            continue;

        int score = scores[i][K - 1];
        if (ans < score)
            ans = score;
    }

    cout << ans << "\n";
    return 0;
}