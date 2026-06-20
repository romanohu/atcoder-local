//#include <bits/stdc++.h>
#include <iostream>
#include <string>
#include <vector>
using namespace std;
using ll = long long;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int N;
    char X;
    cin >> N >> X;

    int int_X = (int)X - 65;

    vector<string> densyas(N);
    for (int i=0; i<N; ++i)
        cin >> densyas[i];

    bool judge = false;
    for (string densya : densyas){
        if (densya[int_X] == 'o'){
            judge = true;
            break;
        }
    }

    if (judge){
        cout << "Yes" << "\n";
    } else {
        cout << "No" << "\n";
    }

    return 0;
}
