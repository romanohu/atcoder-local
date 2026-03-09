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

    vector<int> C(M+1,0);
    for (int i=1; i<M+1; ++i){
        cin >> C[i];
    }

    int ans = 0;
    for (int i=0; i<N; ++i){
        int A, B;
        cin >> A >> B;
        if (C[A] <= 0)
            continue;
        else if (C[A] >= B){
            ans += B;
            C[A] -= B;
        }
        else if (C[A] < B){
            ans += C[A];
            C[A] = 0;
        }
    }

    cout << ans << "\n";


    return 0;
}
