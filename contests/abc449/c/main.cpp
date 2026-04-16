//#include <bits/stdc++.h>
#include <iostream>
#include <vector>
#include <string>
using namespace std;
using ll = long long;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int N, L, R;
    cin >> N >> L >> R;

    string S;
    cin >> S;

    ll cnt = 0;
    vector<int> freq(256, 0);

    for (int j = 0; j < N; ++j) {
        int add = j - L;
        if (add >= 0) {
            freq[(unsigned char)S[add]]++;
        }

        int rem = j - R - 1;
        if (rem >= 0) {
            freq[(unsigned char)S[rem]]--;
        }

        cnt += freq[(unsigned char)S[j]];
    }

    cout << cnt << '\n';
    return 0;
}