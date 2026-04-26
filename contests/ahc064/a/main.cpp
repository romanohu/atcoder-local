#include <algorithm>
#include <array>
#include <chrono>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <tuple>
#include <utility>
#include <vector>

using namespace std;

namespace {

constexpr int R = 10;
constexpr int DEP_CAP = 15;
constexpr int SIDE_CAP = 20;

struct Move {
    int type;
    int i;
    int j;
    int k;
};

struct Turn {
    vector<Move> moves;
};

struct State {
    array<deque<int>, R> dep;
    array<deque<int>, R> side;
};

inline int target_line(int id) {
    return id / 10;
}

inline bool can_type0(const State& st, int i, int j, int k) {
    if (i < 0 || i >= R || j < 0 || j >= R) return false;
    if (k <= 0) return false;
    if ((int)st.dep[i].size() < k) return false;
    if ((int)st.side[j].size() + k > SIDE_CAP) return false;
    return true;
}

inline bool can_type1(const State& st, int i, int j, int k) {
    if (i < 0 || i >= R || j < 0 || j >= R) return false;
    if (k <= 0) return false;
    if ((int)st.side[j].size() < k) return false;
    if ((int)st.dep[i].size() + k > DEP_CAP) return false;
    return true;
}

inline bool apply_move(State& st, const Move& mv) {
    if (mv.type == 0) {
        if (!can_type0(st, mv.i, mv.j, mv.k)) return false;
        for (int t = 0; t < mv.k; t++) {
            int x = st.dep[mv.i].back();
            st.dep[mv.i].pop_back();
            st.side[mv.j].push_front(x);
        }
        return true;
    }

    if (mv.type == 1) {
        if (!can_type1(st, mv.i, mv.j, mv.k)) return false;
        for (int t = 0; t < mv.k; t++) {
            int x = st.side[mv.j].front();
            st.side[mv.j].pop_front();
            st.dep[mv.i].push_back(x);
        }
        return true;
    }

    return false;
}

bool apply_turn(State& st, const Turn& turn) {
    for (const Move& mv : turn.moves) {
        if (!apply_move(st, mv)) return false;
    }
    return true;
}

bool turn_legal(const State& st, const Turn& turn) {
    array<int, R> dep_used{};
    array<int, R> side_used{};
    vector<pair<int, int>> pairs;

    for (const Move& mv : turn.moves) {
        if (mv.i < 0 || mv.i >= R || mv.j < 0 || mv.j >= R || mv.k <= 0) return false;
        if (dep_used[mv.i] || side_used[mv.j]) return false;
        dep_used[mv.i] = 1;
        side_used[mv.j] = 1;
        pairs.push_back({mv.i, mv.j});
    }

    for (int a = 0; a < (int)pairs.size(); a++) {
        for (int b = a + 1; b < (int)pairs.size(); b++) {
            auto [i1, j1] = pairs[a];
            auto [i2, j2] = pairs[b];
            if (!((i1 < i2 && j1 < j2) || (i2 < i1 && j2 < j1))) return false;
        }
    }

    State next = st;
    return apply_turn(next, turn);
}

bool can_merge_turns(const State& st, const Turn& a, const Turn& b) {
    Turn merged;
    merged.moves.reserve(a.moves.size() + b.moves.size());
    merged.moves.insert(merged.moves.end(), a.moves.begin(), a.moves.end());
    merged.moves.insert(merged.moves.end(), b.moves.begin(), b.moves.end());
    return turn_legal(st, merged);
}

struct MoveRef {
    int turn;
    int idx;
};

vector<Turn> merge_same_pair_moves(const vector<Turn>& turns, const State& start) {
    vector<MoveRef> flat;
    for (int t = 0; t < (int)turns.size(); t++) {
        for (int m = 0; m < (int)turns[t].moves.size(); m++) {
            flat.push_back({t, m});
        }
    }
    if (flat.empty()) return turns;

    vector<Turn> out = turns;
    vector<char> alive(flat.size(), 1);
    vector<State> before(flat.size());
    array<int, 2 * R * R> last_same{};
    array<int, R> last_dep_touch{};
    array<int, R> last_side_touch{};
    last_same.fill(-1);
    last_dep_touch.fill(-1);
    last_side_touch.fill(-1);

    auto key_of = [](const Move& mv) {
        return mv.type * R * R + mv.i * R + mv.j;
    };

    State st = start;
    for (int pos = 0; pos < (int)flat.size(); pos++) {
        const MoveRef ref = flat[pos];
        Move& mv = out[ref.turn].moves[ref.idx];
        before[pos] = st;

        int key = key_of(mv);
        int prev = last_same[key];
        bool merged = false;

        if (prev >= 0 && alive[prev] &&
            last_dep_touch[mv.i] <= prev && last_side_touch[mv.j] <= prev) {
            const MoveRef pref = flat[prev];
            Move& pmv = out[pref.turn].moves[pref.idx];
            int nk = pmv.k + mv.k;
            State check = before[prev];
            bool ok = (pmv.type == 0) ? can_type0(check, pmv.i, pmv.j, nk)
                                      : can_type1(check, pmv.i, pmv.j, nk);
            if (ok) {
                pmv.k = nk;
                alive[pos] = 0;
                merged = true;
            }
        }

        if (!apply_move(st, mv)) return turns;

        if (!merged) {
            last_same[key] = pos;
            last_dep_touch[mv.i] = pos;
            last_side_touch[mv.j] = pos;
        }
    }

    vector<Turn> merged_turns;
    merged_turns.reserve(turns.size());
    for (int t = 0; t < (int)turns.size(); t++) {
        Turn nt;
        for (int m = 0; m < (int)out[t].moves.size(); m++) {
            bool keep = true;
            for (int pos = 0; pos < (int)flat.size(); pos++) {
                if (flat[pos].turn == t && flat[pos].idx == m) {
                    keep = alive[pos];
                    break;
                }
            }
            if (keep) nt.moves.push_back(out[t].moves[m]);
        }
        if (!nt.moves.empty()) merged_turns.push_back(std::move(nt));
    }

    return merged_turns;
}

bool shape_compatible(const Turn& turn, const Move& mv) {
    for (const Move& cur : turn.moves) {
        if (cur.i == mv.i || cur.j == mv.j) return false;
        if (!((cur.i < mv.i && cur.j < mv.j) || (mv.i < cur.i && mv.j < cur.j))) return false;
    }
    return true;
}

bool apply_turns(const State& start, const vector<Turn>& turns) {
    State st = start;
    for (const Turn& turn : turns) {
        if (!turn_legal(st, turn)) return false;
        if (!apply_turn(st, turn)) return false;
    }
    return true;
}

vector<Turn> reschedule_moves_earliest(const vector<Turn>& turns, const State& start) {
    vector<Move> flat;
    for (const Turn& turn : turns) {
        for (const Move& mv : turn.moves) flat.push_back(mv);
    }
    if (flat.empty()) return {};

    vector<Turn> packed;
    array<int, R> dep_ready{};
    array<int, R> side_ready{};

    for (const Move& mv : flat) {
        int t = max(dep_ready[mv.i], side_ready[mv.j]);
        while (true) {
            if (t >= (int)packed.size()) packed.resize(t + 1);
            if (shape_compatible(packed[t], mv)) {
                packed[t].moves.push_back(mv);
                dep_ready[mv.i] = t + 1;
                side_ready[mv.j] = t + 1;
                break;
            }
            t++;
        }
    }

    vector<Turn> out;
    out.reserve(packed.size());
    for (Turn& turn : packed) {
        if (!turn.moves.empty()) out.push_back(std::move(turn));
    }

    if (!apply_turns(start, out)) return turns;
    return out;
}

vector<Turn> compress_turns(const vector<Turn>& turns, const State& start) {
    vector<Turn> cur;
    for (const Turn& turn : turns) {
        if (!turn.moves.empty()) cur.push_back(turn);
    }
    if (cur.empty()) return cur;

    bool changed = true;
    while (changed) {
        changed = false;
        vector<Turn> out;
        out.reserve(cur.size());

        State st = start;
        int i = 0;
        while (i < (int)cur.size()) {
            State block_start = st;
            State block_end = st;
            Turn merged = cur[i];
            if (!apply_turn(block_end, merged)) return cur;

            int j = i + 1;
            while (j < (int)cur.size()) {
                if (!can_merge_turns(block_start, merged, cur[j])) break;

                Turn candidate = merged;
                candidate.moves.insert(candidate.moves.end(), cur[j].moves.begin(), cur[j].moves.end());

                State next_end = block_end;
                if (!apply_turn(next_end, cur[j])) break;

                merged = std::move(candidate);
                block_end = std::move(next_end);
                changed = true;
                j++;
            }

            st = std::move(block_end);
            out.push_back(std::move(merged));
            i = j;
        }

        cur.swap(out);
    }

    return cur;
}

vector<int> best_non_crossing_departures(const State& st) {
    array<int, R> to{};
    to.fill(-1);
    for (int i = 0; i < R; i++) {
        if (!st.dep[i].empty()) to[i] = target_line(st.dep[i].back());
    }

    int best_mask = 0;
    int best_cnt = 0;
    for (int mask = 1; mask < (1 << R); mask++) {
        bool ok = true;
        int last_j = -1;
        int cnt = 0;
        for (int i = 0; i < R; i++) {
            if ((mask & (1 << i)) == 0) continue;
            int j = to[i];
            if (j < 0 || j <= last_j) {
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

    vector<int> ret;
    for (int i = 0; i < R; i++) {
        if (best_mask & (1 << i)) ret.push_back(i);
    }
    return ret;
}

struct Phase1Candidate {
    Move mv;
    int score;
};

int side_order_score(const deque<int>& side, int r) {
    vector<int> a;
    a.reserve(side.size());
    for (int x : side) a.push_back(x - 10 * r);

    int score = 0;
    int prefix = 0;
    while (prefix < (int)a.size() && prefix < 10 && a[prefix] == prefix) prefix++;
    score += 500 * prefix;

    for (int i = 0; i + 1 < (int)a.size(); i++) {
        if (a[i + 1] == a[i] + 1) score += 260;
        else if (a[i + 1] > a[i]) score += 25;
        else score -= 45 * (a[i] - a[i + 1]);
    }

    for (int i = 0; i < (int)a.size(); i++) {
        score -= 8 * i * a[i];
        for (int j = i + 1; j < (int)a.size(); j++) {
            if (a[i] > a[j]) score -= 35;
        }
    }

    return score;
}

vector<Move> best_phase1_batch(const State& st) {
    vector<vector<Phase1Candidate>> by_dep(R);

    for (int i = 0; i < R; i++) {
        if (st.dep[i].empty()) continue;
        int j = target_line(st.dep[i].back());
        int max_k = 1;
        while ((int)st.dep[i].size() >= max_k + 1 &&
               target_line(st.dep[i][(int)st.dep[i].size() - max_k - 1]) == j &&
               (int)st.side[j].size() + max_k + 1 <= SIDE_CAP) {
            max_k++;
        }

        int before = side_order_score(st.side[j], j);
        for (int k = 1; k <= max_k; k++) {
            deque<int> next = st.side[j];
            for (int t = 0; t < k; t++) {
                next.push_front(st.dep[i][(int)st.dep[i].size() - 1 - t]);
            }
            int score = side_order_score(next, j) - before;
            by_dep[i].push_back({{0, i, j, k}, score});
        }
    }

    vector<Move> best;
    vector<Move> cur;
    int best_score = -1000000000;
    int best_cars = -1;
    int best_count = -1;

    auto dfs = [&](auto&& self, int i, int last_j, int score, int cars) -> void {
        if (i == R) {
            if (cur.empty()) return;
            int count = (int)cur.size();
            if (count > best_count || (count == best_count && cars > best_cars) ||
                (count == best_count && cars == best_cars && score > best_score)) {
                best_count = count;
                best_score = score;
                best_cars = cars;
                best = cur;
            }
            return;
        }

        self(self, i + 1, last_j, score, cars);
        for (const Phase1Candidate& cand : by_dep[i]) {
            if (cand.mv.j <= last_j) continue;
            cur.push_back(cand.mv);
            self(self, i + 1, cand.mv.j, score + cand.score, cars + cand.mv.k);
            cur.pop_back();
        }
    };

    dfs(dfs, 0, -1, 0, 0);
    return best;
}

vector<Turn> construct_current(State st) {
    State start = st;
    vector<Turn> ans;

    auto append_turn = [&]() {
        ans.push_back(Turn{});
    };

    auto add_move = [&](const Move& mv) -> bool {
        if (!apply_move(st, mv)) return false;
        if (ans.empty()) append_turn();
        ans.back().moves.push_back(mv);
        return true;
    };

    auto start_turn = [&]() {
        if (ans.empty() || !ans.back().moves.empty()) append_turn();
    };

    auto find_next = [&](int r, int need, int& pos, int& take) -> bool {
        int target = 10 * r + need;
        pos = 0;
        while (pos < (int)st.side[r].size() && st.side[r][pos] != target) pos++;
        if (pos >= (int)st.side[r].size()) return false;

        take = 1;
        while (need + take < 10 && pos + take < (int)st.side[r].size() &&
               st.side[r][pos + take] == 10 * r + need + take) {
            take++;
        }
        return true;
    };

    while (true) {
        vector<Move> batch = best_phase1_batch(st);
        if (batch.empty()) break;

        start_turn();
        for (const Move& mv : batch) {
            if (!add_move(mv)) return ans;
        }
    }

    array<int, R> need{};
    int done = 0;
    bool borrow0 = false;

    auto finish_borrow = [&]() -> bool {
        start_turn();
        if (!add_move({1, 0, 0, 10})) return false;
        borrow0 = false;
        return true;
    };

    auto choose_buffer = [&](int r, int pos) {
        int best = -1;
        int best_cost = 1000000;
        for (int b = 0; b < R; b++) {
            if (b == r) continue;
            if ((int)st.dep[b].size() + pos > DEP_CAP) continue;
            int preferred = (r + 1) % R;
            int cost = 100 * abs(b - preferred) + (int)st.dep[b].size();
            if (need[b] >= 10) cost -= 5;
            if (cost < best_cost) {
                best_cost = cost;
                best = b;
            }
        }
        return best;
    };

    auto pick_disjoint_batch = [&](const vector<tuple<int, int, int, int>>& candidates) {
        vector<tuple<int, int, int, int>> best;
        int best_score = -1000000000;
        int best_cnt = -1;
        int m = (int)candidates.size();

        for (int mask = 1; mask < (1 << m); mask++) {
            array<int, R> buf_used{};
            array<int, R> dst_used{};
            vector<pair<int, int>> pairs;
            int score = 0;
            int cnt = 0;
            bool ok = true;

            for (int i = 0; i < m; i++) {
                if ((mask & (1 << i)) == 0) continue;
                int r = get<0>(candidates[i]);
                int pos = get<1>(candidates[i]);
                int take = get<2>(candidates[i]);
                int b = get<3>(candidates[i]);
                if (b == r || buf_used[b] || dst_used[r]) {
                    ok = false;
                    break;
                }
                buf_used[b] = 1;
                dst_used[r] = 1;
                pairs.push_back({b, r});
                score += 1000 * take - 8 * pos - 120 * abs(b - ((r + 1) % R));
                cnt++;
            }
            if (!ok) continue;

            for (int b = 0; b < R; b++) {
                if (buf_used[b] && dst_used[b]) {
                    ok = false;
                    break;
                }
            }
            if (!ok) continue;

            for (int a = 0; a < (int)pairs.size() && ok; a++) {
                for (int b = a + 1; b < (int)pairs.size(); b++) {
                    auto [i1, j1] = pairs[a];
                    auto [i2, j2] = pairs[b];
                    if (!((i1 < i2 && j1 < j2) || (i2 < i1 && j2 < j1))) {
                        ok = false;
                        break;
                    }
                }
            }
            if (!ok) continue;

            if (score > best_score || (score == best_score && cnt > best_cnt)) {
                best_score = score;
                best_cnt = cnt;
                best.clear();
                for (int i = 0; i < m; i++) {
                    if (mask & (1 << i)) best.push_back(candidates[i]);
                }
            }
        }
        return best;
    };

    while (done < R) {
        if (borrow0) {
            int r = R - 1;
            if (need[r] >= 10) continue;

            int pos = 0;
            int take = 0;
            if (!find_next(r, need[r], pos, take)) return ans;

            if (pos > 0) {
                start_turn();
                if (!add_move({1, 0, r, pos})) return ans;
            }

            start_turn();
            if (!add_move({1, r, r, take})) return ans;
            need[r] += take;
            if (need[r] == 10) done++;

            if (pos > 0) {
                start_turn();
                if (!add_move({0, 0, r, pos})) return ans;
            }

            if (need[r] == 10) {
                if (!finish_borrow()) return ans;
            }
            continue;
        }

        vector<pair<int, int>> ready;
        for (int r = 0; r < R; r++) {
            if (need[r] >= 10) continue;
            int pos = 0;
            int take = 0;
            if (!find_next(r, need[r], pos, take)) return ans;
            if (pos == 0) ready.push_back({r, take});
        }

        if (!ready.empty()) {
            start_turn();
            for (auto [r, take] : ready) {
                if (!add_move({1, r, r, take})) return ans;
                need[r] += take;
                if (need[r] == 10) done++;
            }
            continue;
        }

        vector<tuple<int, int, int, int>> candidates;
        for (int r = 0; r < R - 1; r++) {
            if (need[r] >= 10) continue;
            if (r > 0 && need[r - 1] < 10) continue;

            int pos = 0;
            int take = 0;
            if (!find_next(r, need[r], pos, take)) return ans;
            if (pos == 0) continue;
            if ((int)st.dep[r].size() + take > DEP_CAP) continue;
            for (int b = 0; b < R; b++) {
                if (b == r) continue;
                if ((int)st.dep[b].size() + pos > DEP_CAP) continue;
                candidates.push_back({r, pos, take, b});
            }
        }

        vector<tuple<int, int, int, int>> batch = pick_disjoint_batch(candidates);

        if (!batch.empty()) {
            start_turn();
            for (auto [r, pos, take, b] : batch) {
                if (!add_move({1, b, r, pos})) return ans;
            }

            start_turn();
            for (auto [r, pos, take, b] : batch) {
                if (!add_move({1, r, r, take})) return ans;
            }

            start_turn();
            for (auto [r, pos, take, b] : batch) {
                if (!add_move({0, b, r, pos})) return ans;
                need[r] += take;
                if (need[r] == 10) done++;
            }
            continue;
        }

        int r = -1;
        for (int cand = 0; cand < R; cand++) {
            if (need[cand] >= 10) continue;
            if (cand > 0 && need[cand - 1] < 10) continue;
            r = cand;
            break;
        }
        if (r < 0) return ans;

        int pos = 0;
        int take = 0;
        if (!find_next(r, need[r], pos, take)) return ans;
        if ((int)st.dep[r].size() + take > DEP_CAP) return ans;
        int buf = choose_buffer(r, pos);

        if (buf < 0 && r == R - 1 && !borrow0) {
            if ((int)st.dep[0].size() < 10 || (int)st.side[0].size() + 10 > SIDE_CAP) return ans;
            start_turn();
            if (!add_move({0, 0, 0, 10})) return ans;
            borrow0 = true;
            continue;
        }
        if (buf < 0) return ans;

        if (pos > 0) {
            start_turn();
            if (!add_move({1, buf, r, pos})) return ans;
        }

        start_turn();
        if (!add_move({1, r, r, take})) return ans;
        need[r] += take;
        if (need[r] == 10) done++;

        if (pos > 0) {
            start_turn();
            if (!add_move({0, buf, r, pos})) return ans;
        }
    }

    if (borrow0) {
        if (!finish_borrow()) return ans;
    }

    if (!ans.empty() && ans.back().moves.empty()) ans.pop_back();
    vector<Turn> merged = merge_same_pair_moves(ans, start);
    vector<Turn> packed = reschedule_moves_earliest(merged, start);
    return compress_turns(packed, start);
}

int phase1_remaining(const State& st) {
    int rem = 0;
    for (int i = 0; i < R; i++) rem += (int)st.dep[i].size();
    return rem;
}

int phase1_eval(const State& st, int turns) {
    int score = turns * 20000 + phase1_remaining(st) * 300;
    for (int r = 0; r < R; r++) {
        vector<int> a;
        for (int x : st.side[r]) a.push_back(x - 10 * r);

        int prefix = 0;
        while (prefix < (int)a.size() && prefix < 10 && a[prefix] == prefix) prefix++;
        score -= 250 * prefix;

        for (int i = 0; i + 1 < (int)a.size(); i++) {
            if (a[i + 1] == a[i] + 1) score -= 90;
            else if (a[i] > a[i + 1]) score += 25 * (a[i] - a[i + 1]);
        }
        for (int i = 0; i < (int)a.size(); i++) {
            for (int j = i + 1; j < (int)a.size(); j++) {
                if (a[i] > a[j]) score += 20;
            }
        }
    }
    return score;
}

vector<vector<Move>> phase1_batches(const State& st, int limit) {
    vector<vector<Phase1Candidate>> by_dep(R);
    for (int i = 0; i < R; i++) {
        if (st.dep[i].empty()) continue;
        int j = target_line(st.dep[i].back());
        int max_k = min((int)st.dep[i].size(), SIDE_CAP - (int)st.side[j].size());
        while (max_k > 1 && target_line(st.dep[i][(int)st.dep[i].size() - max_k]) != j) max_k--;
        int before = side_order_score(st.side[j], j);

        for (int k = 1; k <= max_k; k++) {
            bool same_target = true;
            for (int t = 0; t < k; t++) {
                if (target_line(st.dep[i][(int)st.dep[i].size() - 1 - t]) != j) same_target = false;
            }
            if (!same_target) break;

            deque<int> next = st.side[j];
            for (int t = 0; t < k; t++) next.push_front(st.dep[i][(int)st.dep[i].size() - 1 - t]);
            int score = 10000 * k + side_order_score(next, j) - before;
            by_dep[i].push_back({{0, i, j, k}, score});
        }
        sort(by_dep[i].begin(), by_dep[i].end(), [](const Phase1Candidate& a, const Phase1Candidate& b) {
            if (a.mv.k != b.mv.k) return a.mv.k > b.mv.k;
            return a.score > b.score;
        });
        if ((int)by_dep[i].size() > 3) by_dep[i].resize(3);
    }

    struct BatchCandidate {
        vector<Move> moves;
        int count = 0;
        int cars = 0;
        int score = 0;
    };

    vector<BatchCandidate> cand;
    vector<Move> cur;
    auto dfs = [&](auto&& self, int i, int last_j, int count, int cars, int score) -> void {
        if (i == R) {
            if (!cur.empty()) cand.push_back({cur, count, cars, score});
            return;
        }

        self(self, i + 1, last_j, count, cars, score);
        for (const Phase1Candidate& c : by_dep[i]) {
            if (c.mv.j <= last_j) continue;
            cur.push_back(c.mv);
            self(self, i + 1, c.mv.j, count + 1, cars + c.mv.k, score + c.score);
            cur.pop_back();
        }
    };
    dfs(dfs, 0, -1, 0, 0, 0);

    sort(cand.begin(), cand.end(), [](const BatchCandidate& a, const BatchCandidate& b) {
        if (a.count != b.count) return a.count > b.count;
        if (a.cars != b.cars) return a.cars > b.cars;
        return a.score > b.score;
    });
    if ((int)cand.size() > limit) cand.resize(limit);

    vector<vector<Move>> ret;
    ret.reserve(cand.size());
    for (auto& c : cand) ret.push_back(std::move(c.moves));
    return ret;
}

vector<Turn> solve(State start) {
    vector<Turn> best = construct_current(start);
    int best_size = (int)best.size();

    struct BeamNode {
        State st;
        vector<Turn> turns;
        int eval = 0;
    };

    const auto deadline = chrono::steady_clock::now() + chrono::milliseconds(1600);
    constexpr int BEAM_WIDTH = 90;
    constexpr int BATCH_LIMIT = 24;
    constexpr int COMPLETE_LIMIT = 80;

    vector<BeamNode> beam;
    beam.push_back({start, {}, phase1_eval(start, 0)});
    int completed = 0;

    while (!beam.empty() && chrono::steady_clock::now() < deadline) {
        vector<BeamNode> next_beam;

        for (const BeamNode& node : beam) {
            if (chrono::steady_clock::now() >= deadline) break;

            if (phase1_remaining(node.st) == 0) {
                if (completed < COMPLETE_LIMIT) {
                    vector<Turn> tail = construct_current(node.st);
                    vector<Turn> cand = node.turns;
                    cand.insert(cand.end(), tail.begin(), tail.end());
                    vector<Turn> merged = merge_same_pair_moves(cand, start);
                    vector<Turn> packed = reschedule_moves_earliest(merged, start);
                    cand = compress_turns(packed, start);
                    if ((int)cand.size() < best_size) {
                        best_size = (int)cand.size();
                        best = std::move(cand);
                    }
                    completed++;
                }
                continue;
            }

            vector<vector<Move>> batches = phase1_batches(node.st, BATCH_LIMIT);
            for (const vector<Move>& batch : batches) {
                State ns = node.st;
                bool ok = true;
                for (const Move& mv : batch) {
                    if (!apply_move(ns, mv)) {
                        ok = false;
                        break;
                    }
                }
                if (!ok) continue;

                BeamNode nxt;
                nxt.st = std::move(ns);
                nxt.turns = node.turns;
                nxt.turns.push_back(Turn{batch});
                nxt.eval = phase1_eval(nxt.st, (int)nxt.turns.size());
                next_beam.push_back(std::move(nxt));
            }
        }

        if (next_beam.empty()) break;
        sort(next_beam.begin(), next_beam.end(), [](const BeamNode& a, const BeamNode& b) {
            if (a.eval != b.eval) return a.eval < b.eval;
            return a.turns.size() < b.turns.size();
        });
        if ((int)next_beam.size() > BEAM_WIDTH) next_beam.resize(BEAM_WIDTH);
        beam.swap(next_beam);
    }

    return best;
}

}  // namespace

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int r_in;
    if (!(cin >> r_in)) return 0;
    if (r_in != R) return 0;

    State st;
    for (int r = 0; r < R; r++) {
        for (int c = 0; c < 10; c++) {
            int x;
            cin >> x;
            st.dep[r].push_back(x);
        }
    }

    vector<Turn> ans = solve(st);
    cout << ans.size() << '\n';
    for (const auto& turn : ans) {
        cout << turn.moves.size() << '\n';
        for (const auto& mv : turn.moves) {
            cout << mv.type << ' ' << mv.i << ' ' << mv.j << ' ' << mv.k << '\n';
        }
    }
    return 0;
}
