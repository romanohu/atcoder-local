//#include <bits/stdc++.h>
#include <iostream>
# include <stack>
#include <deque>
using namespace std;
using ll = long long;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int Q;
    cin >> Q;

    stack<int> st(std::deque<int>(100, 0));

    for (int i=0; i<Q; ++i){
        int q1, q2;
        cin >> q1;

        if (q1 == 1){
            cin >> q2;
            st.push(q2);
        } else {
            cout << st.top() << "\n";
            st.pop();
        }
    }


    return 0;
}
