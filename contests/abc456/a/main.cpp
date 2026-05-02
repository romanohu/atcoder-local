//#include <bits/stdc++.h>
#include <iostream>
using namespace std;
using ll = long long;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int X;
    cin >> X;

    if ((X > 2) & (X < 19)){
        cout << "Yes" << "\n";
        return 0;
    }

    cout << "No" << "\n";

    return 0;
}
