#include <bits/stdc++.h>
using namespace std;
using ll = long long;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int N, M, Q;
    cin >> N >> M;
    string S, T;
    cin >> S >> T;
    cin >> Q;

    for (int i=0; i<Q; ++i)
    {
        string w;
        cin >> w;
        while(!w.empty())
        {
            char w_front = w.front();
            w.erase(0, 1);
            if ((S.find(w_front) != string::npos) && (T.find(w_front) != string::npos))
            {
                if (w.empty())
                {
                    cout << "Unknown" << "\n";
                    break;
                }
                continue;
            } else if (S.find(w_front) != string::npos) {
                cout << "Takahashi" << "\n";
                break;    
            } else if (T.find(w_front) != string::npos) {
                cout << "Aoki" << "\n";
                break;
            }
        }
    }


    return 0;
}
