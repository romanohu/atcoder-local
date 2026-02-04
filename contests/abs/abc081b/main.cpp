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

    vector<int> A(N);
    for (int i = 0; i < N; ++i)
    {
        cin >> A[i];
    }

    int cnt = 0;
    while (true)
    {
        for (int i = 0; i < N; ++i){
            if ((A[i] % 2 != 0) && (A[i] != 0) ){
                cout << cnt << "\n";
                return 0;
            }
        }
        
        for (int i = 0; i < N; ++i) {
            A[i] /= 2;
        }
        cnt++;
    }

    return 0;
}
