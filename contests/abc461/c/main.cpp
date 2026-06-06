#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;

using ll = long long;
const ll NEG = -(1LL << 60);

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int N, K, M;
    cin >> N >> K >> M;

    vector<vector<int>> group(K);

    for (int i = 0; i < N; i++){
        int c, v;
        cin >> c >> v;
        group[c].push_back(v);
    }

    for (int i = 0; i < K; i++){
        sort(group[i].rbegin(), group[i].rend());
    }

    return 0;
}