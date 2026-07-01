//#include <bits/stdc++.h>
#include <iostream>
#include <vector>
using namespace std;
using ll = long long;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int N;
    cin >> N;

    vector<int> cnt(N+1,0), ans;

    for (int i=0; i<N*3; ++i){
        int c;
        cin >> c;
        cnt[c]++;
        if (cnt[c] == 2)
            ans.push_back(c);
    }

    for (int i=0; i<N; ++i){
        cout << ans[i] << (i == N-1 ? "\n" : " ");
    }



    

    return 0;
}
