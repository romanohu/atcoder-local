//#include <bits/stdc++.h>
#include <iostream>
#include <string>
#include <vector>
using namespace std;
using ll = long long;


pair<string, vector<int>> decompose(const string &s){
    string t;
    vector<int> v;
    int cnt = 0;
    for(char c: s){
        if (c == 'A'){
            ++cnt;
        } else {
            t += c;
            v.push_back(cnt);
            cnt = 0;
        }
    }
    v.push_back(cnt);
    return {t, v};
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    string S, T;
    cin >> S >> T;

    auto [ss ,sv] = decompose(S);
    auto [ts, tv] = decompose(T);

    if (ss != ts){
        cout << -1 << "\n";
        return 0;
    }

    int ans = 0;
    for (int i=0; i<(int)sv.size(); ++i){
        ans += abs(sv[i] - tv[i]);
    }
    cout << ans << "\n";
    return 0;
}
