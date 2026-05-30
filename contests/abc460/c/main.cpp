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

    vector<int> sharis(N);
    for (int i=0; i<N; ++i)
        cin >> sharis[i];
    vector<int> netas(M);
    for (int i=0; i<M; ++i)
        cin >> netas[i];


    sort(sharis.begin(), sharis.end());
    sort(netas.begin(), netas.end());

    int ans = 0;

    for (int shari : sharis){
        for (int j=ans; j<M; ++j){
            if (shari * 2 >= netas[j])
                ans++;
                break;
        }
    }

    cout << ans << "\n";

    return 0;
}
