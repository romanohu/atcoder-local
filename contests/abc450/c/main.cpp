//#include <bits/stdc++.h>
#include <iostream> 
#include <vector> 
#include <queue>

using namespace std;

bool bfs(const vector<vector<char>>& graph, vector<vector<bool>>& visited, pair<int, int> start) {
    int H = graph.size();
    int W = graph[0].size();
    queue<pair<int, int>> q;

    visited[start.first][start.second] = true;
    q.push(start);

    bool closed = true;  // 境界に触れていなければ true

    int dh[4] = {-1, 1, 0, 0};
    int dw[4] = {0, 0, -1, 1};

    while (!q.empty()) {
        auto [y, x] = q.front();
        q.pop();

        if (y == 0 || y == H - 1 || x == 0 || x == W - 1) {
            closed = false;
        }

        for (int dir = 0; dir < 4; ++dir) {
            int ny = y + dh[dir];
            int nx = x + dw[dir];

            if (ny < 0 || ny >= H || nx < 0 || nx >= W) continue;
            if (visited[ny][nx]) continue;
            if (graph[ny][nx] == '#') continue;

            visited[ny][nx] = true;
            q.push({ny, nx});
        }
    }

    return closed;
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int H, W;
    cin >> H >> W;

    vector<vector<char>> mp(H, vector<char>(W));
    for (int h = 0; h < H; ++h) {
        for (int w = 0; w < W; ++w) {
            cin >> mp[h][w];
        }
    }

    vector<vector<bool>> visited(H, vector<bool>(W, false));
    int cnt = 0;

    for (int h = 0; h < H; ++h) {
        for (int w = 0; w < W; ++w) {
            if (visited[h][w] || mp[h][w] == '#') continue;

            if (bfs(mp, visited, {h, w})) {
                cnt++;
            }
        }
    }

    cout << cnt << "\n";
    return 0;
}