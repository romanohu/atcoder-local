//#include <bits/stdc++.h>
#include <iostream>
using namespace std;
using ll = long long;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int N, M, x;
    cin >> N >> M;

    int cnt = 0;
    x = N;

    while (x != 0)
    {
        x = N % M;
        M = x;
        cnt++;
    }
    
    cout << cnt << "\n";

    return 0;
}
