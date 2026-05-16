//#include <bits/stdc++.h>
#include <iostream>
#include <string>
using namespace std;
using ll = long long;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    string S;
    int N;
    cin >> S >> N;

    int S_size = (int)S.size();

    string ans = S.substr(N, S_size - 2 * N);
    cout << ans << "\n";

    return 0;
}
