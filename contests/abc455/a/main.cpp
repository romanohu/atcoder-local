//#include <bits/stdc++.h>
#include <iostream>
using namespace std;
using ll = long long;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int A, B, C;
    cin >> A >> B >> C;
    if ((A != B) and (B == C)){
        cout << "Yes" << "\n";
        return 0;
    }

    cout << "No" << "\n";

    return 0;
}
