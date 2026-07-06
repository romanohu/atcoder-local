//#include <bits/stdc++.h>
#include <iostream>
using namespace std;
using ll = long long;

#define rep(i, N) for (int i=0; i<(N); ++i)

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int a, b, c;
    cin >> a >> b >> c;

    if (a <= b && b <= c){
        cout << "Yes" << "\n";
    } else if (a >= b && b >= c){
        cout << "Yes" << "\n";
    } else {
        cout << "No" << "\n";
    }

    return 0;
}
