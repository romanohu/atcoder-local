//#include <bits/stdc++.h>
#include <iostream>
#include <string>
using namespace std;
using ll = long long;

string judge(ll X1, ll Y1, ll R1, ll X2, ll Y2, ll R2) {
    ll dx = X2 - X1;
    ll dy = Y2 - Y1;

    ll d2 = dx * dx + dy * dy;

    ll rsum = R1 + R2;
    ll rdiff = abs(R1 - R2);

    if (rdiff * rdiff <= d2 && d2 <= rsum * rsum)
        return "Yes";
    else
        return "No";
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int T;
    cin >> T;

    for (int i = 0; i < T; ++i) {
        ll X1, Y1, R1, X2, Y2, R2;
        cin >> X1 >> Y1 >> R1 >> X2 >> Y2 >> R2;
        cout << judge(X1, Y1, R1, X2, Y2, R2) << "\n";
    }

    return 0;
}