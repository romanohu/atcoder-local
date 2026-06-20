//#include <bits/stdc++.h>
#include <iostream>
using namespace std;
using ll = long long;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int X, Y;
    cin >> X >> Y;

    if (X * 9 == Y * 16){
        cout << "Yes" << "\n";
        return 0;
    }

    cout << "No" << "\n";

    return 0;
}
