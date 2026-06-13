//#include <bits/stdc++.h>
#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;
//using ll = long long;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int N;
    cin >> N;

    vector<pair<int, int>> XY(N);
    for (int i = 0; i < N; ++i){
        cin >> XY[i].first >> XY[i].second;
    }

    sort(XY.begin(), XY.end());

    int ans = 0;
    int minY = N;

    for (int i = 0; i < N; ++i){
        int y = XY[i].second;

        if (y <= minY)
            ans++;

        minY = min(minY, y);
    }

    cout << ans << '\n';
    return 0;
}