//#include <bits/stdc++.h>
#include <iostream>
#include <algorithm>
#include <functional>

using namespace std;
using ll = long long;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int N;
    cin >> N;
    vector<int> A(N);

    for (int i=0; i<N; ++i){
        cin >> A[i];
    }
    
    sort(A.begin(), A.end(), greater<int>());
    
    int Alice = 0, Bob = 0;
    bool turn = true;
    for (int i=0; i<N; ++i){
        if (turn){
            Alice += A[i];
            turn = false;
        } else {
            Bob += A[i];
            turn = true;
        }
    }

    cout << Alice - Bob << "\n";
    return 0;
}
