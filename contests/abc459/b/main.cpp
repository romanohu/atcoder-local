//#include <bits/stdc++.h>
#include <iostream>
#include <string>
#include <vector>
using namespace std;
using ll = long long;

char judge(char head){
    if (head == 'a' or head == 'b' or head == 'c')
        return '2';
    else if (head == 'd' or head == 'e' or head == 'f')
        return '3';
    else if (head == 'g' or head == 'h' or head == 'i')
        return '4';
    else if (head == 'j' or head == 'k' or head == 'l')
        return '5';
    else if (head == 'm' or head == 'n' or head == 'o')
        return '6';
    else if (head == 'p' or head == 'q' or head == 'r' or head == 's')
        return '7';
    else if (head == 't' or head == 'u' or head == 'v')
        return '8';
    else
        return '9';
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int N;
    cin >> N;
    vector<string> words(N, "");

    for (int i=0; i<N; ++i){
        cin >> words[i];
    }

    string ans = "";

    for (string word : words){
        ans += judge(word[0]);
    }

    cout << ans << "\n";


    return 0;
}
