//#include <bits/stdc++.h>
#include <iostream>
#include <vector>
using namespace std;
using ll = long long;

#define rep(i, N) for (int i=0; i<(N); ++i)

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int N;
    cin >> N;

    vector<int> A(N);
    rep(i, N) cin >> A[i];

    vector<int> B(N);
    rep(i, N) cin >> B[i];

    bool judge = false;
    rep(i, N){
        if (i + 1 == B[A[i] - 1])
            continue;
        else{
            judge = true;
            break;
        }
    }
    
    if (judge)
        cout << "No" << "\n";
    else
        cout << "Yes" << "\n";

    return 0;
}
