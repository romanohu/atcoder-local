//#include <bits/stdc++.h>
#include <iostream>
#include <cmath>
using namespace std;
using ll = long long;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int N;
    ll now, cnt;
    cin >> N;
    cnt = 0;
    now = 1;

    for (int i=0; i<N; ++i){
        ll step;
        cin >> step;
        if ((now - step) > 0.5){
            now -= step;
        } else {
            now = abs(step - now);
            cnt++;
        }
    }

    cout << cnt << "\n";


    return 0;
}
