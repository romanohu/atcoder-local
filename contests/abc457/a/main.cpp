//#include <bits/stdc++.h>
#include <iostream>
#include <vector>
using namespace std;
using ll = long long;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int N,X;
    cin >> N;

    vector<int> A(N+1,0);
    for (int i=0; i<N; ++i){
        cin >> A[i+1];
    }

    cin >> X;

    cout << A[X] << "\n";

    return 0;
}
