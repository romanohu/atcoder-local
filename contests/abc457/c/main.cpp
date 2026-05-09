#include <iostream>
#include <vector>
using namespace std;
using ll = long long;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int N;
    ll K;
    cin >> N >> K;

    vector<vector<int>> A(N);
    vector<int> C(N);

    for (int i = 0; i < N; ++i) {
        int L;
        cin >> L;

        A[i].resize(L);

        for (int j = 0; j < L; ++j) {
            cin >> A[i][j];
        }
    }

    for (int i = 0; i < N; ++i) {
        cin >> C[i];
    }

    vector<ll> total_list(N + 1, 0);

    for (int i = 1; i <= N; ++i) {
        total_list[i] =
            total_list[i - 1]
            + 1LL * C[i - 1] * A[i - 1].size();
    }

    ll pred = 0;
    int ans = -1;

    for (int i = 1; i <= N; ++i) {
        ll now = total_list[i];

        if (pred < K && K <= now) {

            ll offset = K - pred - 1;

            int in_index = offset % A[i - 1].size();

            ans = A[i - 1][in_index];
            break;
        }

        pred = now;
    }

    cout << ans << '\n';

    return 0;
}