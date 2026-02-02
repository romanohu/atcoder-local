//#include <bits/stdc++.h>
#include <iostream>
using namespace std;
using ll = long long;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int s, sum = 0;
    cin >> s;

    while (s > 0)
    {
        sum += s % 10;
        s /= 10;
    }

    cout << sum << "\n";

    return 0;
}
