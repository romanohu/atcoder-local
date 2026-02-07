// #include <bits/stdc++.h>
#include <iostream>
#include <vector>
#include <string>

using namespace std;
using ll = long long;

int main()
{
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int N;
    cin >> N;
    vector<int> A(N);
    for (int i = 0; i < N; ++i)
    {
        cin >> A[i];
    }

    ll total = 0;

    for (int i = 0; i < N; ++i)
    {
        ll num = 0;
        for (int j = 0; j < A[i]; ++j)
        {
            num = num * 10 + 1;
        }
        total += num;
    }

    cout << total << "\n";

    return 0;
}
