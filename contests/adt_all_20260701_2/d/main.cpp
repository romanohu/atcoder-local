//#include <bits/stdc++.h>
#include <iostream>
#include <string>
#include <vector>
using namespace std;
using ll = long long;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int N, K;
    cin >> N >> K;
    
    string S;
    cin >> S;

    vector<int> has;
    
    int ha_cnt = 0;
    for (char ha : S){
        if (ha == 'X'){
            if (ha_cnt > 0){
                has.push_back(ha_cnt);
            }
            ha_cnt = 0;
        } else {
            ha_cnt++;
        }
    }
    has.push_back(ha_cnt);

    int itigo_cnt = 0;
    for (int i : has){
        while (i >= K)
        {
            itigo_cnt++;
            i -= K;
        }
    }

    cout << itigo_cnt << "\n";

    return 0;
}
