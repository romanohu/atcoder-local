//#include <bits/stdc++.h>
#include <iostream>
#include <string>
#include <cctype>
using namespace std;
using ll = long long;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    string S;
    cin >> S;

    string ans = "";
    for (char c : S){
        if (isdigit(static_cast<unsigned char>(c)))
            ans += c;
        else
            continue;
    }

    cout << ans << "\n";

    return 0;
}
