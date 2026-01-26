#include <bits/stdc++.h>
using namespace std;
using ll = long long;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int N, K;
    ll X;
    cin >> N >> K >> X;

    vector<ll> A(N);
    for (int i=0; i<N; ++i)
        cin >> A[i];
    sort(A.begin(), A.end());

    vector<ll> prefix_sum(N+1, 0);
    for (int i=0; i<N; ++i){
        prefix_sum[i+1] = prefix_sum[i] + A[i];
    }

    for (int m=1; m<= N; ++m){
        int m_sake = m - (N - K);

        if (m_sake <= 0){
            continue;
        }

        ll current_sake = prefix_sum[N - m + m_sake] - prefix_sum[N - m];

        if (current_sake >= X){
            cout << m << "\n";
            return 0;
        }
    }

    cout << -1 << "\n";

    return 0;
}
