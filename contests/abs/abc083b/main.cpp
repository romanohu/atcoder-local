//#include <bits/stdc++.h>
#include <iostream>
using namespace std;
using ll = long long;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int N, A, B;
    int total = 0;
    cin >> N >> A >> B;

    for (int i=1; i<=N; ++i){
        int number = i;
        int cnt = 0;

        while (number>0)
        {
            cnt += number % 10;
            number /= 10;
        }

        if ((cnt >= A) && (cnt <= B)){
            total += i;
        }
    }

    cout << total << "\n";

    return 0;
}
