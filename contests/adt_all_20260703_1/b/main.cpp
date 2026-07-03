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

    bool judge = false;
    for (char s : S){
        if (s == 'o'){
            judge = true;
        } else if (s == 'x'){
            judge = false;
            break;
        } else {
            continue;
        }
    }

    if (judge)
        cout << "Yes" << "\n";
    else
        cout << "No" << "\n";

    return 0;
}
