// #include <bits/stdc++.h>
#include <iostream>
#include <string>
#include <vector>
using namespace std;
using ll = long long;

int main()
{
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int N, max = 0;
    cin >> N;
    vector<string> S(N, "");
    for (int i = 0; i < N; ++i)
    {
        cin >> S[i];
        int len = S[i].length();
        if (len > max)
            max = len;
    }

    for (int i = 0; i < N; ++i)
    {
        int loop = (max - S[i].length()) / 2;
        for (int j = 0; j < loop; ++j)
        {
            S[i].insert(0, ".");
            S[i].append(".");
        }
    }

    for (int i = 0; i < N; ++i)
    {
        cout << S[i] << "\n";
    }
}
