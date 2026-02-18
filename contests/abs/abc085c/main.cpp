//#include <bits/stdc++.h>
#include <iostream>
using namespace std;
using ll = long long;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    ll N, Y;
    cin >> N >> Y;

    for (int z=0; z<=N; ++z){
        for (int y=0; y<=N-z; ++y){
            for (int x=0; x<=N-y-z; ++x){
                ll sum = (10000 * x) + (5000 * y) + (1000 * z);
                if ((sum == Y) && ((x + y + z) == N)){
                    cout << x << " " << y << " " << z << "\n";
                    return 0;
                }
            }
        }
    }

    cout << "-1 -1 -1" << "\n";
    return 0;
}
