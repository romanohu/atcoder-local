#include <bits/stdc++.h>
using namespace std;
using ll = long long;

int main()
{
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int N, riko_min, riko_max;
    cin >> N;
    if (N % 2 == 0)
    {
        riko_min = N / 2;
        riko_max = N;
    }
    else
    {
        riko_min = N / 2 + 1;
        riko_max = N;
    }

    vector<int> A(N);
    for (int i = 0; i < N; ++i)
    {
        cin >> A[i];
    }

    for (int i=riko_min; i<riko_max; ++i){}

    return 0;
}
