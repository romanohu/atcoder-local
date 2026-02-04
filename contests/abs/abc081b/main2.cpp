#include <iostream>
#include <algorithm>
#include <vector>

using namespace std;

int main() {
    int N;
    cin >> N;

    int ans = 1000000; 

    for (int i = 0; i < N; ++i) {
        int a;
        cin >> a;

        if (a == 0) {
            continue; 
        }
        // 2進数の末尾にゼロがいくつあるかを返す
        int count = __builtin_ctz(a);
        ans = min(ans, count);
    }

    cout << ans << endl;

    return 0;
}