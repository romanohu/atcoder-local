#include <iostream>
#include <string>
#include <vector>
using namespace std;
using ll = long long;


int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    string s;
    cin >> s;

    // vector<int> compressed;

    // int count = 1;

    // for (int i = 1; i < (int)s.size(); i++) {
    //     if (s[i] == s[i - 1]) {
    //         count++;
    //     } else {
    //         compressed.push_back(count);
    //         count = 1;
    //     }
    // }

    // compressed.push_back(count);

    // int right = 0;
    // int size = (int)compressed.size();

    // ll sum = 0;

    // for (int left = 0; left < size; ++left) {
    //     while (right < size && compressed[right] < 2) {
    //         sum++;
    //         right++;
    //     }

    //     if (right < size) {
    //         sum += compressed[right];
    //         ++sum;
    //     }

    //     if (right == left) {
    //         ++right;
    //     } else {
    //         sum -= compressed[left];
    //     }
    // }

    // ll ans = sum % 998244353;
    // if (ans < 0) ans += 998244353;

    // ll ans = 1;

    // for (int x : compressed) {
    //     ans = ans * (x + 1) % 998244353;
    // }

    // ans = (ans - 1 + 998244353) % 998244353;

    ll ans = 0;
    ll len = 0;

    for (int i = 0; i < (int)s.size(); i++) {
        if (i == 0 || s[i] != s[i - 1]) {
            len++;
        } else {
            len = 1;
        }

        ans += len;
        ans %= 998244353;
    }

    cout << ans << "\n";

    return 0;
}