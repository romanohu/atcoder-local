//#include <bits/stdc++.h>
#include <iostream>
#include <string>
#include <vector>

using namespace std;
using ll = long long;

vector<vector<int>> calcNext(const string &S) {
    int n = (int)S.size();
    vector<vector<int>> res(n + 1, vector<int>(3, n));

    for (int i = n - 1; i >= 0; --i) {
        for (int c = 0; c < 3; ++c) {
            res[i][c] = res[i + 1][c];
        }
        res[i][S[i] - 'a'] = i;
    }

    return res;
}


void add(long long &a, long long b) {
    a += b;
    if (a >= 998244353) a -= 998244353;
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    string S;
    cin >> S;

    int n = (int)S.size();

    vector<vector<int> > next = calcNext(S);

    vector<vector<long long>> dp(n + 1, vector<long long>(4, 0));

    dp[0][3] = 1;

    for (int i = 0; i < n; ++i) {
        for (int last = 0; last <= 3; ++last) {
            if (dp[i][last] == 0) continue;

            for (int c = 0; c < 3; ++c) {
                if (c == last) continue;

                int pos = next[i][c];
                if (pos >= n) continue;

                add(dp[pos + 1][c], dp[i][last]);
            }
        }
    }

    long long res = 0;
    for (int i = 0; i <= n; ++i) {
        for (int last = 0; last <= 3; ++last) {
            add(res, dp[i][last]);
        }
    }

    cout << res << "\n";




    return 0;
}
