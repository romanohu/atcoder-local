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
    string S;
    cin >> S;

    if (N % 2 == 0){
        cout << "No" << "\n";
        return 0;
    }


    bool judge = true;
    for (int i=1; i<=N; ++i){
        if (((N+1)/2) > i){
            if (S[i-1] != '1'){
                judge = false;
                break;}}
        else if (((N+1)/2) == i){
            if (S[i-1] != '/'){
                judge = false;
                break;}}
        else
            if (S[i-1] != '2'){
                judge = false;
                break;}
    }

    if (judge)
        cout << "Yes" << "\n";
    else
        cout << "No" << "\n";

    return 0;
}
