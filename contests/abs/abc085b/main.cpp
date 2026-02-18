//#include <bits/stdc++.h>
#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;
using ll = long long;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int N;
    cin >> N;
    vector<int> D(N,0);
    for (int i=0; i<N; ++i){
        cin >> D[i];
    }
    sort(D.begin(), D.end());
    
    int cnt = 1;
    int pre = D[0];
    for (int i=1; i<N; ++i){
        int now = D[i];
        if (now == pre)
            continue;
        pre = now;
        cnt++;
    }

    cout << cnt << "\n";
    return 0;
}
