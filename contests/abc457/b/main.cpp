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

    vector<vector<int>> A(N);
    for (int i=0; i<N; ++i){
        int L;
        cin >> L;
        for (int l=0; l<L; ++l){
            int enter;
            cin >> enter;
            A[i].push_back(enter);
        }
    }
    int X, Y;
    cin >> X >> Y;
    cout << A[X-1][Y-1] << '\n';

    return 0;
}
