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

    vector<int> A(N + 1);
    for (int i=1; i<=N; ++i)
        cin >> A[i];
    
    int X;
    cin >> X;

    cout << A[X] << "\n";

    return 0;
}
