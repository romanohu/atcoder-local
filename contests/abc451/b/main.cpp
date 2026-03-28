//#include <bits/stdc++.h>
#include <iostream>
#include <vector>
using namespace std;
using ll = long long;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int N, M;
    cin >> N >> M;

    vector<int> ans(M + 1, 0);
    for (int i=0; i<N; ++i){
        int now, next;
        cin >> now >> next;
        ans[now] -= 1;
        ans[next] += 1;
    }

    for (int i=1; i<=M; ++i){
        cout << ans[i] << "\n";
    }


    return 0;
}
