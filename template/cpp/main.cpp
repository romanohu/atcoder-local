//#include <bits/stdc++.h>
#include <iostream>
#include <string>
#include <vector>
using namespace std;
using ll = long long;

#define rep(i, N) for (int i=0; i<(N); ++i)

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int N;
    cin >> N;

    string T, A;
    cin >> T >> A;

    bool judge = false;
    rep(i, N){
        if (T[i] == 'o' && A[i] == 'o'){
            judge = true;
            break;
        }
    }

    if (judge){
        cout << "Yes" << "\n";}
    else{
        cout << "No" << "\n";}


    return 0;
}
