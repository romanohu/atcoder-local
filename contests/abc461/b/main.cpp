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

    vector<int> A(N+1);
    vector<int> B(N+1);

    for (int i=1; i<=N; ++i)
        cin >> A[i];
    for (int i=1; i<=N; ++i)
        cin >> B[i];
    
    for (int i=1; i<=N; ++i){
        int ax = A[i];
        if (B[ax] != i){
            cout << "No" << "\n";
            return 0;
        }
    }

    cout << "Yes" << "\n";

    return 0;
}
