#include <bits/stdc++.h>
using namespace std;
using ll = long long;

int main()
{
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int N, D;
    if (!(cin >> N >> D))
        return 0;

    vector<int> A(N);
    vector<vector<int>> sum_matrix(N, vector<int>(N));

    for (int i = 0; i < N; ++i)
    {
        cin >> A[i];
    }

    for (int i = 0; i < N; ++i)
    {
        for (int j = 0; j < N; ++j)
        {
            sum_matrix[i][j] = abs(A[i] - A[j]);
        }
    }

    int total = 0;
    bool judge = false;
    for (int i = 0; i < N; ++i)
    {
        for (int j = 0; j < N; ++j)
        {
            if (!(sum_matrix[i][j] >= D))
            {
                continue;
            }
            judge = true;
        }
        if (judge)
        {
            total++;
        }
        judge = false;
    }

    cout << total << "\n";

    return 0;
}
