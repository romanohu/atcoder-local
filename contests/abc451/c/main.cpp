//#include <bits/stdc++.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <functional>
#include <queue>
using namespace std;
using ll = long long;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int Q;
    cin >> Q;

    priority_queue<int, vector<int>, greater<int>> trees;

    for (int i=0; i<Q; ++i){
        int query;
        int h;
        cin >> query >> h;
        if (query == 1){
            trees.push(h);
            cout << int(trees.size()) << "\n";
        }
        else{
           int size = trees.size();
           for (int j=0; j<size; ++j){
            if (trees.top() <= h){
                trees.pop();
                continue;
            } else{
                break;
            }
           }
           cout << trees.size() << "\n";
        }
        
    }

    return 0;
}
