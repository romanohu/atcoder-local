#include <iostream>
#include <tuple>
#include <utility>
#include <vector>
using namespace std;

struct Operation {
    int belt;
    int dir;
};

int N;
int exit_col;
int vertical_belt;
vector<vector<pair<int, int>>> belts;
vector<vector<int>> board;
vector<pair<int, int>> pos;
vector<Operation> operations;
int next_box = 0;

bool on_vertical_belt(int c) {
    return c == exit_col || c == exit_col + 1;
}

int index_on_belt(int belt_id, int r, int c) {
    const auto &belt = belts[belt_id];
    for (int i = 0; i < (int)belt.size(); i++) {
        if (belt[i] == make_pair(r, c)) return i;
    }
    return -1;
}

pair<int, int> best_rotation(int len, int cur, const vector<int> &goals) {
    int best_steps = len + 1;
    int best_dir = 1;

    for (int goal : goals) {
        int plus_steps = (goal - cur + len) % len;
        if (plus_steps < best_steps) {
            best_steps = plus_steps;
            best_dir = 1;
        }

        int minus_steps = (cur - goal + len) % len;
        if (minus_steps < best_steps) {
            best_steps = minus_steps;
            best_dir = -1;
        }
    }

    return {best_steps, best_dir};
}

void remove_if_ready() {
    if (next_box < N * N && board[0][exit_col] == next_box) {
        pos[next_box] = {-1, -1};
        board[0][exit_col] = -1;
        next_box++;
    }
}

bool rotate_belt(int belt_id, int dir) {
    const int MAX_OPERATIONS = 100000;
    if ((int)operations.size() >= MAX_OPERATIONS) return false;

    const auto &belt = belts[belt_id];
    int len = belt.size();
    vector<int> old_values(len);

    for (int i = 0; i < len; i++) {
        auto [r, c] = belt[i];
        old_values[i] = board[r][c];
    }

    for (int i = 0; i < len; i++) {
        int ni = (i + dir + len) % len;
        auto [nr, nc] = belt[ni];
        board[nr][nc] = old_values[i];
        if (old_values[i] != -1) {
            pos[old_values[i]] = {nr, nc};
        }
    }

    operations.push_back({belt_id, dir});
    remove_if_ready();
    return true;
}

void build_belts() {
    // Build 2-row loops.
    for (int r = 0; r < N; r += 2) {
        vector<pair<int, int>> belt;
        for (int c = 0; c < N; c++) {
            belt.push_back({r, c});
        }
        for (int c = N - 1; c >= 0; c--) {
            belt.push_back({r + 1, c});
        }
        belts.push_back(belt);
    }

    // Build one vertical loop through the exit.
    vertical_belt = belts.size();
    vector<pair<int, int>> belt;
    for (int r = 0; r < N; r++) {
        belt.push_back({r, exit_col});
    }
    for (int r = N - 1; r >= 0; r--) {
        belt.push_back({r, exit_col + 1});
    }
    belts.push_back(belt);
}

void move_current_box() {
    int target = next_box;
    auto [r, c] = pos[target];

    if (!on_vertical_belt(c)) {
        int belt_id = r / 2;
        int cur = index_on_belt(belt_id, r, c);
        vector<int> goals;

        for (int i = 0; i < (int)belts[belt_id].size(); i++) {
            auto [br, bc] = belts[belt_id][i];
            if (on_vertical_belt(bc)) goals.push_back(i);
        }

        auto [steps, dir] = best_rotation((int)belts[belt_id].size(), cur, goals);
        for (int i = 0; i < steps && next_box == target; i++) {
            if (!rotate_belt(belt_id, dir)) return;
        }
    }

    if (next_box != target) return;

    tie(r, c) = pos[target];
    int cur = index_on_belt(vertical_belt, r, c);
    int goal = index_on_belt(vertical_belt, 0, exit_col);
    auto [steps, dir] = best_rotation((int)belts[vertical_belt].size(), cur, {goal});

    for (int i = 0; i < steps && next_box == target; i++) {
        if (!rotate_belt(vertical_belt, dir)) return;
    }
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    cin >> N;
    exit_col = N / 2;
    board.assign(N, vector<int>(N));
    pos.assign(N * N, {-1, -1});

    for (int r = 0; r < N; r++) {
        for (int c = 0; c < N; c++) {
            cin >> board[r][c];
            pos[board[r][c]] = {r, c};
        }
    }

    build_belts();
    remove_if_ready();

    while (next_box < N * N && (int)operations.size() < 100000) {
        move_current_box();
    }

    cout << belts.size() << '\n';
    for (const auto &belt : belts) {
        cout << belt.size();
        for (auto [r, c] : belt) {
            cout << ' ' << r << ' ' << c;
        }
        cout << '\n';
    }

    cout << operations.size() << '\n';
    for (auto [belt, dir] : operations) {
        cout << belt << ' ' << dir << '\n';
    }

    return 0;
}
