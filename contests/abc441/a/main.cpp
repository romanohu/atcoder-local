#include <bits/stdc++.h>
using namespace std;
using ll = long long;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    ll P, Q, X, Y;
    cin >> P >> Q >> X >> Y;

    if ((P <= X ) && (X < P + 100) && (Q <= Y) && (Y < Q + 100))
    {
        cout << "Yes" << "\n";
        return 0;
    }

    cout << "No" << "\n";

    return 0;
}
