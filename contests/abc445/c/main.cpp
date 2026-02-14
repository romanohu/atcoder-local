// #include <bits/stdc++.h>
#include <iostream>
#include <vector>
using namespace std;
using ll = long long;

int main()
{
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    ll N;
    cin >> N;
    vector<ll> S(N + 1, 0);
    for (int i = 0; i < N; ++i)
        cin >> S[i + 1];

    bool first = true;

    for (ll i = 1; i <= N; ++i)
    {
        ll pre = N + 1;
        ll now = S[i];
        bool judge = false;
        for (int j = 0; j < 10; ++j)
        {
            for (int k = 0; k < 100; ++k)
            {
                if ((now == pre))
                {
                    judge = true;
                    break;
                }
                pre = now;
                now = S[pre];
            }
            if (judge)
                break;
        }
        if (first)
        {
            cout << now;
            first = false;
        }
        else
        {
            cout << " " << now;
        }
    }

    cout << "\n";
    return 0;
}
