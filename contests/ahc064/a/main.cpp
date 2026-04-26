#include <deque>
#include <iostream>
#include <vector>

using namespace std;

struct Move {
    int type;
    int i;
    int j;
    int k;
};

struct Turn {
    vector<Move> moves;
};

static bool can_type0(const vector<deque<int>>& dep, const vector<deque<int>>& side, int i, int j, int k, int R) {
    if (i < 0 || i >= R || j < 0 || j >= R) return false;
    if (k <= 0) return false;
    if ((int)dep[i].size() < k) return false;
    if ((int)side[j].size() + k > 20) return false;
    return true;
}

static bool can_type1(const vector<deque<int>>& dep, const vector<deque<int>>& side, int i, int j, int k, int R) {
    if (i < 0 || i >= R || j < 0 || j >= R) return false;
    if (k <= 0) return false;
    if ((int)side[j].size() < k) return false;
    if ((int)dep[i].size() + k > 15) return false;
    return true;
}

// For each departure line i, choose a maximal set of cars to move in one turn.
// Constraint: selected pairs (i, j=i->j) in increasing i must have strictly increasing j,
// and each line is used at most once.
static vector<int> best_non_crossing_batch(const vector<deque<int>>& dep, int R) {
    vector<int> target(R, -1);
    for (int i = 0; i < R; i++) {
        if (!dep[i].empty()) {
            target[i] = dep[i].back() / 10;
        }
    }

    int best_mask = 0;
    int best_cnt = 0;
    const int max_mask = 1 << R;

    for (int mask = 1; mask < max_mask; mask++) {
        bool ok = true;
        int last_j = -1;
        int cnt = 0;

        for (int i = 0; i < R; i++) {
            if (!(mask & (1 << i))) continue;
            int j = target[i];
            if (j < 0) {
                ok = false;
                break;
            }
            if (j <= last_j) {
                ok = false;
                break;
            }
            last_j = j;
            cnt++;
        }

        if (ok && cnt > best_cnt) {
            best_cnt = cnt;
            best_mask = mask;
        }
    }

    vector<int> picked;
    for (int i = 0; i < R; i++) {
        if (best_mask & (1 << i)) picked.push_back(i);
    }
    return picked;
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int R;
    if (!(cin >> R)) return 0;

    vector<deque<int>> dep(R), side(R);
    for (int r = 0; r < R; r++) {
        for (int c = 0; c < 10; c++) {
            int x;
            cin >> x;
            dep[r].push_back(x);
        }
    }

    vector<Turn> turns;
    auto append_new_turn = [&]() {
        turns.push_back(Turn{});
    };
    auto add_move = [&](int type, int i, int j, int k) {
        if (turns.empty()) append_new_turn();
        turns.back().moves.push_back({type, i, j, k});
    };
    auto flush_turn = [&]() {
        append_new_turn();
    };

    auto do_type0 = [&](int i, int j, int k) -> bool {
        if (!can_type0(dep, side, i, j, k, R)) return false;
        for (int t = 0; t < k; t++) {
            int val = dep[i].back();
            dep[i].pop_back();
            side[j].push_front(val);
        }
        return true;
    };

    auto do_type1 = [&](int i, int j, int k) -> bool {
        if (!can_type1(dep, side, i, j, k, R)) return false;
        for (int t = 0; t < k; t++) {
            int val = side[j].front();
            side[j].pop_front();
            dep[i].push_back(val);
        }
        return true;
    };

    // Start with one active turn.
    append_new_turn();

    // Phase 1: move every car to its destination side track.
    // We compress this phase by taking at most one move from each departure line per turn,
    // and choosing the largest non-crossing subset (indices i and destinations j are increasing).
    while (true) {
        vector<int> batch = best_non_crossing_batch(dep, R);
        if (batch.empty()) break;

        // Each turn collects all moves in batch.
        turns.back().moves.clear();
        for (int i : batch) {
            int j = dep[i].back() / 10;
            if (!do_type0(i, j, 1)) {
                cerr << "Invalid move in phase 1" << '\n';
                return 0;
            }
            add_move(0, i, j, 1);
        }
        // Next batch should be a new turn.
        flush_turn();
    }

    // Remove trailing empty turn created at loop end.
    if (!turns.empty() && turns.back().moves.empty()) {
        turns.pop_back();
    }

    // Phase 2: for each side track r, bring IDs to dep[r] in order 10r..10r+9.
    // To place 10r+need, move prefix [0..pos-1] to buffer dep line, move target, then restore prefix.
    for (int r = 0; r < R; r++) {
        int buf = (r + 1) % R;
        bool borrowed = false;

        // For the last destination line, dep[0] already holds final state and is used as temp by rotating.
        if (r == R - 1 && !dep[0].empty()) {
            if ((int)side[0].size() + 10 > 20) {
                cerr << "Capacity exceeded when borrowing dep[0]" << '\n';
                return 0;
            }
            append_new_turn();
            if (!do_type0(0, 0, 10)) {
                cerr << "Invalid borrow operation" << '\n';
                return 0;
            }
            turns.back().moves.push_back({0, 0, 0, 10});
            buf = 0;
            borrowed = true;
        }

        for (int need = 0; need < 10; need++) {
            int target = 10 * r + need;
            int pos = 0;
            while (pos < (int)side[r].size() && side[r][pos] != target) {
                pos++;
            }
            if (pos >= (int)side[r].size()) {
                cerr << "Target id not found" << '\n';
                return 0;
            }

            if (pos > 0) {
                append_new_turn();
                if (!do_type1(buf, r, pos)) {
                    cerr << "Invalid extraction" << '\n';
                    return 0;
                }
                turns.back().moves.push_back({1, buf, r, pos});
            }

            append_new_turn();
            if (!do_type1(r, r, 1)) {
                cerr << "Invalid move to destination" << '\n';
                return 0;
            }
            turns.back().moves.push_back({1, r, r, 1});

            if (pos > 0) {
                append_new_turn();
                if (!do_type0(buf, r, pos)) {
                    cerr << "Invalid restore" << '\n';
                    return 0;
                }
                turns.back().moves.push_back({0, buf, r, pos});
            }
        }

        if (borrowed) {
            append_new_turn();
            if (!do_type1(0, 0, 10)) {
                cerr << "Invalid restore of borrowed line" << '\n';
                return 0;
            }
            turns.back().moves.push_back({1, 0, 0, 10});
        }
    }

    cout << turns.size() << '\n';
    for (const auto& turn : turns) {
        cout << turn.moves.size() << '\n';
        for (const auto& mv : turn.moves) {
            cout << mv.type << ' ' << mv.i << ' ' << mv.j << ' ' << mv.k << '\n';
        }
    }
    return 0;
}
