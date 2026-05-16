#include <iostream>
#include <vector>
using namespace std;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int H, W;
    cin >> H >> W;

    vector<vector<int>> ans(H, vector<int>(W, 0));

    int dh[4] = {-1, 1, 0, 0};
    int dw[4] = {0, 0, -1, 1};

    for (int h = 0; h < H; ++h) {
        for (int w = 0; w < W; ++w) {
            int cnt = 0;
            for (int k = 0; k < 4; ++k) {
                int nh = h + dh[k];
                int nw = w + dw[k];

                if (0 <= nh && nh < H && 0 <= nw && nw < W) {
                    cnt++;
                }
            }
            ans[h][w] = cnt;
        }
    }

    for (int h = 0; h < H; ++h) {
        for (int w = 0; w < W; ++w) {
            if (w){
                cout << ' ';
            }
            cout << ans[h][w];
        }
        cout << '\n';
    }

    return 0;
}