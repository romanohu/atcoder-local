//#include <bits/stdc++.h>
#include <iostream>
#include <vector>
# include <algorithm>
#include <iomanip>
using namespace std;
using ll = long long;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    vector<vector<double>> A(3, vector<double>(6,0.0));

    for (int i=0; i<3; ++i){
        for (int j=0; j<6; ++j){
            cin >> A[i][j];
        }
    }
    
    vector<double> A1 = A[0];
    vector<double> A2 = A[1];
    vector<double> A3 = A[2];

    double A1_4 = count(A1.begin(), A1.end(), 4.0), A1_5 =  count(A1.begin(), A1.end(), 5.0), A1_6 = count(A1.begin(), A1.end(), 6.0);
    double A2_4 = count(A2.begin(), A2.end(), 4.0), A2_5 =  count(A2.begin(), A2.end(), 5.0), A2_6 = count(A2.begin(), A2.end(), 6.0);
    double A3_4 = count(A3.begin(), A3.end(), 4.0), A3_5 =  count(A3.begin(), A3.end(), 5.0), A3_6 = count(A3.begin(), A3.end(), 6.0);

    double prob = 0.0;
    prob += (A1_4 / 6.0) * (A2_5 / 6.0) * (A3_6 / 6.0);
    prob += (A1_4 / 6.0) * (A2_6 / 6.0) * (A3_5 / 6.0);
    prob += (A1_5 / 6.0) * (A2_4 / 6.0) * (A3_6 / 6.0);
    prob += (A1_5 / 6.0) * (A2_6 / 6.0) * (A3_4 / 6.0);
    prob += (A1_6 / 6.0) * (A2_5 / 6.0) * (A3_4 / 6.0);
    prob += (A1_6 / 6.0) * (A2_4 / 6.0) * (A3_5 / 6.0);

    cout << fixed << setprecision(10) << prob << "\n";

    return 0;
}
