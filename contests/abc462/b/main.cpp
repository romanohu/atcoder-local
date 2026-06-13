//#include <bits/stdc++.h>
#include <iostream>
#include <vector>
#include <algorithm>
# include <functional>
using namespace std;
using ll = long long;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int N;
    cin >> N;

    vector<vector<int>> hitobito(N+1);

    for (int i=1; i<=N; i++){
        int K;
        cin >> K;
        for (int j=1; j<=K; ++j){
            int A;
            cin >> A;
            hitobito[A].push_back(i);
        }
    }

    for (int i=1; i<=N; ++i){
        sort(hitobito[i].begin(), hitobito[i].end());
        int size = (int)hitobito[i].size();
        cout << size;
        for (int j=0; j<size; ++j){
            cout << " " << hitobito[i][j];
        }
        cout << "\n";
    }




    return 0;
}
