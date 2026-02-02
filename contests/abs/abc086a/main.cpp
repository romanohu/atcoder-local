//#include <bits/stdc++.h>
#include <iostream>
using namespace std;
using ll = long long;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int a, b;
    cin >> a >> b;

    string judge = "Even";
    if ((a%2 != 0) && (b%2 != 0))
    {
        judge = "Odd";
    }

    cout << judge << "\n";
    return 0;
}
