//#include <bits/stdc++.h>
#include <iostream>
#include <vector>
using namespace std;
using ll = long long;

bool check(const vector<vector<char>> &map, int h1, int h2, int w1, int w2){
    for (int h=h1; h<=h2; ++h){
        for (int w=w1; w<=w2; ++w){
            if (map[h][w] != map[h1 + h2 - h][w1 + w2 - w]){
                return false;
            }
        }
    }
    return true;
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int H, W;
    cin >> H >> W;

    vector<vector<char>> map(H, vector<char>(W, ' '));
    for (int h=0; h<H; ++h){
        for (int w=0; w<W; ++w){
            cin >> map[h][w];
        }
    }

    int cnt = 0;

    for (int h=0; h<H; ++h){
        for (int w=0; w<W; ++w){
            for (int h2=h; h2<H; ++h2){
                for (int w2=w; w2<W; ++w2){
                    if (check(map, h, h2, w, w2)){
                        cnt++;
                    }
                }
            }
        }
    }


    cout << cnt << "\n";


    return 0;
}
