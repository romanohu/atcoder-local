//#include <bits/stdc++.h>
#include <iostream>
#include <vector>
# include <queue>
# include <functional>
using namespace std;
using ll = long long;

priority_queue<int> Q_low;
priority_queue<int, vector<int>, greater<int>> Q_high;

void Q_add(int x) {
    if (Q_low.empty() or x <= Q_low.top()) {
        Q_low.push(x);
    } else {
        Q_high.push(x);
    }


    if (Q_low.size() < Q_high.size()) {
        Q_low.push(Q_high.top());
        Q_high.pop();
    }
    if (Q_low.size() > Q_high.size() + 1) {
        Q_high.push(Q_low.top());
        Q_low.pop();
    }
}


int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int X, Q;
    cin >> X >> Q;

    vector<vector<int>> query(Q, vector<int>(2,0));
    for (int i=0; i<Q; ++i){
        int a, b;
        cin >> a >> b;
        query[i][0] = a;
        query[i][1] = b;
    }

    Q_add(X);

    for (int i = 0; i < Q; ++i) {
        int a = query[i][0], b = query[i][1];

        Q_add(a);
        Q_add(b);

        cout <<  Q_low.top() << '\n';
    }

    return 0;
}