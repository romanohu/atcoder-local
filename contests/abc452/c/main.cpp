//#include <bits/stdc++.h>
#include <iostream>
#include <vector>
using namespace std;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int N;
    cin >> N;

    vector<int> A(N), B(N);
    for (int i = 0; i < N; i++) {
        cin >> A[i] >> B[i];
    }

    int M;
    cin >> M;
    vector<string> S(M);
    for (int i = 0; i < M; i++) {
        cin >> S[i];
    }

    // exist[len][pos][c]
    // len: 1..10, pos: 1..10, c: 0..25
    bool exist[11][11][26] = {};

    for (const string& s : S) {
        int len = (int)s.size();
        for (int k = 0; k < len; k++) {
            exist[len][k + 1][s[k] - 'a'] = true;
        }
    }

    for (const string& t : S) {
        if ((int)t.size() != N) {
            cout << "No\n";
            continue;
        }

        bool ok = true;
        for (int i = 0; i < N; i++) {
            if (!exist[A[i]][B[i]][t[i] - 'a']) {
                ok = false;
                break;
            }
        }

        cout << (ok ? "Yes\n" : "No\n");
    }

    return 0;
}