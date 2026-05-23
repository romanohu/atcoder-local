//#include <bits/stdc++.h>
#include <iostream>
#include <string>
using namespace std;
using ll = long long;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int X;
    cin >> X;

    string h = "HelloWorld";
    string ans ="";

    for (int i=1; i<=h.size(); ++i){
        if (i == X){
            continue;
        }
        char moji = h[i-1];
        ans += moji;
    }

    cout << ans << "\n";


    return 0;
}
