//#include <bits/stdc++.h>
#include <iostream>
#include <string>
using namespace std;
using ll = long long;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    string S;
    cin >> S;

    int ans = (int)(S[0] - '0') * (int)(S[2] - '0');
    cout << ans << "\n";

    return 0;
}
