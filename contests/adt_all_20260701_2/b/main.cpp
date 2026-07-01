//#include <bits/stdc++.h>
#include <iostream>
using namespace std;
using ll = long long;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int A, B, C, D;
    cin >> A >> B >> C >> D;

    bool judge = true;
    
    if (A<C){
        judge = false;
    } else if(A == C){
        if (B <= D){
            judge = false;
        }
    }

    if (!judge){
        cout << "Takahashi" << "\n";
    } else {
        cout << "Aoki" << "\n";
    }

    return 0;
}
