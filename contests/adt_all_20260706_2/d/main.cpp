//#include <bits/stdc++.h>
#include <iostream>
#include <string>
#include <cctype>
using namespace std;
using ll = long long;

#define rep(i, N) for (int i=0; i<(N); ++i)

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    string S, T;
    cin >> S >> T;

    bool judge = true;
    rep(i, S.size()){
        char s = S[i];
        if (isupper(static_cast<unsigned char>(s)) && i != 0){
            char pre_s = S[i-1];
            rep(j, T.size()){
                if (pre_s == T[j])
                    break;
                else if (j == T.size()-1)
                    judge = false;
            }
        }
    }

    if (judge)
        cout << "Yes" << "\n";
    else
        cout << "No" << "\n";


    return 0;
}
