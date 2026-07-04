//#include <bits/stdc++.h>
#include <iostream>
using namespace std;
using ll = long long;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int X, Y, L, R, A, B;
    cin >> X >> Y >> L >> R >> A >> B;

    int in_cnt = 0;
    for (int i=A; i<B; ++i){
        if (i >= L && i < R)
            in_cnt++;
    }

    cout << (B-A-in_cnt) * Y + in_cnt * X << "\n";

    return 0;
}
