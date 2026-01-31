#include <bits/stdc++.h>
using namespace std;
using ll = long long;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    ll N, K;
    cin >> N >> K;
    
    int cnt = 0; 
    ll sum_val = N;

    while (sum_val < K){
        N++;
        sum_val += N;
        cnt++;
    }

    cout << cnt << "\n";

    return 0;
}
