//#include <bits/stdc++.h>
#include <iostream>
using namespace std;
using ll = long long;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int N;
    cin >> N;

    int R = 0, L = 0;
    int R_pos = -1, L_pos = -1;
    for (int i=0; i<N; ++i){
        int A;
        char S;
        cin >> A >> S;

        if (S == 'R'){
            if (R_pos < 0)
                R_pos = A;
            R += abs(R_pos - A);
        }
        else {
            if (L_pos < 0)
                L_pos = A;
            L += abs(L_pos - A);
        }
    }

    cout << R + L << "\n";

    return 0;
}
