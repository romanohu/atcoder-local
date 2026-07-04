//#include <bits/stdc++.h>
#include <iostream>
using namespace std;
using ll = long long;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int A, B;
    cin >> A >> B;

    if (A * 3 > B *2)
        cout << "Yes" << "\n";
    else
        cout << "No" << "\n";

    return 0;
}
