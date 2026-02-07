#include <bits/stdc++.h>
using namespace std;
using ll = long long;

int main()
{
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int N, K;
    cin >> N >> K;

    int total = 0;

    for (int i = 1; i <= N; ++i)
    {
        int number = i;
        int cnt = 0;

        while (number > 0)
        {
            cnt += number % 10;
            number /= 10;
        }

        if (cnt == K)
            total++;
    }

    cout << total << "\n";
    return 0;
}
