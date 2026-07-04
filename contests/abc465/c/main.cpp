//#include <bits/stdc++.h>
#include <iostream>
#include <string>
#include <deque>
using namespace std;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int N;
    cin >> N;

    string S;
    cin >> S;

    deque<int> dq;
    bool rev = false;

    for (int i = 1; i <= N; ++i){
        if (S[i - 1] == 'o'){
            rev = !rev;

            if (rev) 
                dq.push_back(i);
            else 
                dq.push_front(i);
        } else {
            if (rev) 
                dq.push_front(i);
            else 
                dq.push_back(i);
        }
    }

    for (int i = 0; i < N; ++i){
        int x;
        if (!rev){
            x = dq.front();
            dq.pop_front();
        } else{
            x = dq.back();
            dq.pop_back();
        }

        cout << x << (i == N - 1 ? '\n' : ' ');
    }

    return 0;
}