//#include <bits/stdc++.h>
#include <iostream>
#include <vector>
#include <map>
using namespace std;
using ll = long long;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int N, Q;
    cin >> N >> Q;

    vector<int> blocks(N, 0);
    map<int, int> cnt;
    cnt[0] = N;

    int remove_cnt = 0;

    for (int i = 0; i < Q; ++i) {
        int q, num;
        cin >> q >> num;

        if (q == 1) {
            int idx = num - 1;

            cnt[blocks[idx]]--;
            if (cnt[blocks[idx]] == 0) {
                cnt.erase(blocks[idx]);
            }

            blocks[idx]++;
            cnt[blocks[idx]]++;

            if (cnt.begin()->first - remove_cnt >= 1) {
                remove_cnt++;
            }
        }
        else {
            int threshold = num + remove_cnt;
            int ans_cnt = 0;

            for (auto it = cnt.lower_bound(threshold); it != cnt.end(); ++it) {
                ans_cnt += it->second;
            }

            cout << ans_cnt << "\n";
        }
    }

    return 0;
}