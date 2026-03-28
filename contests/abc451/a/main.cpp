//#include <bits/stdc++.h>
#include <iostream>
using namespace std;
using ll = long long;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    string S;
    cin >> S;
    if (S.size() % 5 == 0){
        cout << "Yes" << "\n";
        return 0;
    } 

    cout << "No" << "\n";
    return 0;
}
