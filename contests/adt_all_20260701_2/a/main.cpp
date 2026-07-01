//#include <bits/stdc++.h>
#include <iostream>
#include <string>
using namespace std;
using ll = long long;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int N;
    cin >> N;

    bool judge = true;
    bool pre = false;

    for (int i=0; i<N-1; ++i){
        string S;
        cin >> S;
        if (S == "sweet"){
            if (pre){
                judge = false;
                break;
            }
            else{
                pre = true;
            }
        } else {
            pre = false;
        }
    }

    if (judge){
        cout << "Yes" << "\n";
    }
    else {
        cout << "No" << "\n";
    }

    return 0;
}
