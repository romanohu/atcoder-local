//#include <bits/stdc++.h>
#include <iostream>
#include <vector>
#include <cmath>
using namespace std;
using ll = long long;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int T, X, save;
    cin >> T >> X >> save;
    cout << 0 << " " << save << "\n";

    for (int i=1; i<=T; ++i){
        int now;
        cin >> now;
        if (abs(save - now) >= X){
            save = now;
            cout << i << " " << save << "\n";
        }
    }


    return 0;
}
