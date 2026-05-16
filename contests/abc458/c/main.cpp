//#include <bits/stdc++.h>
#include <iostream>
#include <string>
using namespace std;
using ll = long long;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    string S;
    cin >> S;

    int S_size = (int)S.size();
    int ans = 0;

    for (int i=0; i<S_size; ++i){
        char moji = S[i];
        if (moji == 'C'){
            int end = S_size - i - 1;
            if (i < end){
                ans += i + 1;
            }
            else{
                ans += end + 1;
            }
        }
    }

    cout << ans << "\n";

    return 0;
}
