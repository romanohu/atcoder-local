#include <bits/stdc++.h>
using namespace std;
using ll = long long;

int main()
{
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int N, judge1 = 0, judge2 = 0, judge3 = 0;
    cin >> N;

    judge1 = N % 10;
    N /= 10;
    judge2 = N % 10;
    N /= 10;
    judge3 = N % 10;

    if ((judge1 == judge2) && (judge2 == judge3))
    {
        cout << "Yes" << "\n";
        return 0;
    }

    cout << "No" << "\n";
    return 0;
}
