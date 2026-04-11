//#include <bits/stdc++.h>
#include <iostream>
#include <string>
using namespace std;
using ll = long long;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int N, cnt;
    string S;
    cin >> N >> S;
    cnt = 0;
    for (int i=0; i<N; ++i){
        if (S[i] == 'o'){
            cnt++;}
        else
            break;
    }

    cout << S.substr(cnt) << '\n';


    return 0;
}
