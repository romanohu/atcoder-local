#include <algorithm>
#include <chrono>
#include <functional>
#include <iostream>
#include <queue>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>
using namespace std;

#ifndef HIER_CAND_LIMIT
#define HIER_CAND_LIMIT 1000
#endif

#ifndef HIER_MAX_LEN
#define HIER_MAX_LEN 100
#endif

#ifndef HIER_START_LIMIT
#define HIER_START_LIMIT 90
#endif

#ifndef SOLVER_TIME_LIMIT_MS
#define SOLVER_TIME_LIMIT_MS 1300
#endif

#ifndef ORDER_TIME_LIMIT_MS
#define ORDER_TIME_LIMIT_MS 1050
#endif

#ifndef ORDER_BEAM_WIDTH
#define ORDER_BEAM_WIDTH 40
#endif

#ifndef ORDER_BEAM_KEEP
#define ORDER_BEAM_KEEP 40
#endif

#ifndef IMPROVE_FIRST_ROUNDS
#define IMPROVE_FIRST_ROUNDS 80
#endif

#ifndef IMPROVE_CANDIDATE_ROUNDS
#define IMPROVE_CANDIDATE_ROUNDS 12
#endif

#ifndef ROUTE_F_COST
#define ROUTE_F_COST 100
#endif

#ifndef ROUTE_TURN_COST
#define ROUTE_TURN_COST 100
#endif

#ifndef ENABLE_SCAN_CANDIDATE
#define ENABLE_SCAN_CANDIDATE 0
#endif

#ifndef ENABLE_PATTERN_ORDERS
#define ENABLE_PATTERN_ORDERS 0
#endif

#ifndef ENABLE_CARRY_PATTERN_ORDERS
#define ENABLE_CARRY_PATTERN_ORDERS 0
#endif

#ifndef ENABLE_ALT_ROUTE_CANDIDATE
#define ENABLE_ALT_ROUTE_CANDIDATE 0
#endif

struct Pos {
    int r;
    int c;
};

struct Route {
    int cost = 1000000000;
    int end_dir = -1;
    string ops;
};

struct PrevStep {
    int prev = -1;
    int len = 0;
    int end = 0;
    bool macro = false;
};

struct BuiltAnswer {
    string base;
    int delivered = 0;
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int N, M, T;
    cin >> N >> M >> T;

    vector<string> v(N), h(N - 1);
    for (int i = 0; i < N; ++i) cin >> v[i];
    for (int i = 0; i < N - 1; ++i) cin >> h[i];

    int wall_count = 0;
    for (const string &row : v) wall_count += count(row.begin(), row.end(), '1');
    for (const string &row : h) wall_count += count(row.begin(), row.end(), '1');

    vector<Pos> ball(M), basket(M);
    for (int k = 0; k < M; ++k) {
        cin >> ball[k].r >> ball[k].c >> basket[k].r >> basket[k].c;
    }

    const auto start_time = chrono::steady_clock::now();
    auto elapsed_ms = [&]() -> long long {
        return chrono::duration_cast<chrono::milliseconds>(
                   chrono::steady_clock::now() - start_time)
            .count();
    };
    auto time_over = [&]() -> bool {
        return elapsed_ms() >= SOLVER_TIME_LIMIT_MS;
    };
    auto order_time_over = [&]() -> bool {
        return elapsed_ms() >= ORDER_TIME_LIMIT_MS;
    };

    const int dr[4] = {-1, 0, 1, 0};
    const int dc[4] = {0, 1, 0, -1};

    auto can_move = [&](int r, int c, int dir) -> bool {
        if (dir == 0) return r > 0 && h[r - 1][c] == '0';
        if (dir == 1) return c + 1 < N && v[r][c] == '0';
        if (dir == 2) return r + 1 < N && h[r][c] == '0';
        return c > 0 && v[r][c - 1] == '0';
    };

    auto id_of = [&](int r, int c, int dir) {
        return ((r * N + c) << 2) | dir;
    };

    auto decode = [&](int id, int &r, int &c, int &dir) {
        dir = id & 3;
        int cell = id >> 2;
        r = cell / N;
        c = cell % N;
    };

    int tie_mode = 0;  // 0: LRF, 1: LFR, 2: RFL, 3: RLF, 4: FRL, 5: FLR
    int wall_density = 1000 * wall_count / (2 * N * N - 2 * N);
    int tnm = 100 * T / (2 * N * N * M);
    int t_per_n = T / N;
    if (wall_density > 178) {
        tie_mode = 1;
    } else if (wall_count <= 39) {
        if (wall_count <= 38) {
            if (T <= 3108) {
                if (t_per_n <= 156) {
                    if (tnm <= 72) {
                        tie_mode = 0;
                    } else {
                        tie_mode = 2;
                    }
                } else if (wall_count <= 21) {
                    tie_mode = 1;
                } else {
                    tie_mode = 2;
                }
            } else {
                tie_mode = 3;
            }
        } else {
            if (tnm <= 27) {
                if (wall_density <= 127 || N <= 13) {
                    tie_mode = 2;
                } else {
                    tie_mode = 3;
                }
            } else if (wall_count <= 42) {
                if (N <= 13) {
                    tie_mode = 5;
                } else {
                    tie_mode = 1;
                }
            } else if (wall_count <= 82) {
                if (T <= 1335) {
                    tie_mode = 4;
                } else {
                    tie_mode = 2;
                }
            } else {
                tie_mode = 5;
            }
        }
    } else if (tnm <= 27) {
        if (wall_density <= 127) {
            tie_mode = 2;
        } else if (N <= 13) {
            tie_mode = 2;
        } else {
            tie_mode = 3;
        }
    } else if (wall_count <= 42) {
        if (N <= 13) {
            tie_mode = 5;
        } else {
            tie_mode = 1;
        }
    } else if (wall_count <= 82) {
        if (T <= 1335) {
            tie_mode = 4;
        } else {
            tie_mode = 2;
        }
    } else {
        tie_mode = 5;
    }

    if (tie_mode == 2 && T <= 3234) {
        tie_mode = 1;
    }
    if (tie_mode == 1 && t_per_n <= 163) {
        tie_mode = 2;
    }
    if (tie_mode == 0 && wall_count <= 7) {
        tie_mode = 4;
    }
    if (tie_mode == 1 && wall_count > 105) {
        tie_mode = 4;
    }

    if (N <= 14 && M >= 25) {
        tie_mode = 5;
    } else if (210 <= t_per_n && t_per_n <= 216) {
        tie_mode = 4;
    } else if (T >= 15404) {
        tie_mode = 0;
    } else if (319 <= t_per_n && t_per_n <= 379) {
        tie_mode = 2;
    } else if (81 <= tnm && tnm <= 85) {
        tie_mode = 1;
    } else if (T <= 877 && t_per_n >= 80) {
        tie_mode = 2;
    } else if (T <= 2361 && t_per_n >= 196) {
        tie_mode = 5;
    } else if (wall_count <= 18 && wall_density >= 49) {
        tie_mode = 2;
    } else if (N <= 11 && T >= 1909 && t_per_n >= 224) {
        tie_mode = 3;
    } else if (T <= 1404 && wall_density <= 80) {
        tie_mode = 1;
    } else if (15 <= wall_count && wall_count <= 25) {
        tie_mode = 0;
    } else if (M >= 25 && wall_density <= 51) {
        tie_mode = 4;
    } else if (47 <= wall_count && wall_count <= 48) {
        tie_mode = 0;
    }

#ifdef FORCE_TIE_MODE
    tie_mode = FORCE_TIE_MODE;
#endif

    const bool macro_priority_mode = (tnm <= 77 && t_per_n >= 319);
    const bool light_order_mode =
        (!macro_priority_mode && T <= 2511 && wall_density >= 125 && N >= 11);

    vector<Pos> nodes;
    nodes.push_back({0, 0});
    for (int k = 0; k < M; ++k) nodes.push_back(ball[k]);
    for (int k = 0; k < M; ++k) nodes.push_back(basket[k]);

    const int node_count = static_cast<int>(nodes.size());
    vector<vector<int>> nodes_at_cell(N * N);
    for (int i = 0; i < node_count; ++i) {
        nodes_at_cell[nodes[i].r * N + nodes[i].c].push_back(i);
    }

    vector<vector<vector<Route>>> route(
        node_count, vector<vector<Route>>(4, vector<Route>(node_count)));

    auto build_routes = [&](vector<vector<vector<Route>>> &table, int mode, int source, int sdir) {
        const int states = N * N * 4;
        vector<int> prev(states, -1);
        vector<char> prev_op(states, 0);
        vector<int> best_state(node_count, -1);

#if ROUTE_F_COST != ROUTE_TURN_COST
        const int inf = 1000000000;
        vector<int> dist(states, inf);
        priority_queue<pair<int, int>, vector<pair<int, int>>, greater<pair<int, int>>> que;

        int start = id_of(nodes[source].r, nodes[source].c, sdir);
        dist[start] = 0;
        prev[start] = start;
        que.push({0, start});

        while (!que.empty()) {
            auto [cur_dist, cur] = que.top();
            que.pop();
            if (cur_dist != dist[cur]) continue;

            int r, c, dir;
            decode(cur, r, c, dir);
            for (int target : nodes_at_cell[r * N + c]) {
                if (best_state[target] == -1) best_state[target] = cur;
            }

            vector<pair<char, int>> nexts;
            auto add_left = [&]() { nexts.push_back({'L', id_of(r, c, (dir + 3) & 3)}); };
            auto add_right = [&]() { nexts.push_back({'R', id_of(r, c, (dir + 1) & 3)}); };
            auto add_forward = [&]() {
                if (can_move(r, c, dir)) {
                    nexts.push_back({'F', id_of(r + dr[dir], c + dc[dir], dir)});
                }
            };

            if (mode == 0) {
                add_left();
                add_right();
                add_forward();
            } else if (mode == 1) {
                add_left();
                add_forward();
                add_right();
            } else if (mode == 2) {
                add_right();
                add_forward();
                add_left();
            } else if (mode == 3) {
                add_right();
                add_left();
                add_forward();
            } else if (mode == 4) {
                add_forward();
                add_right();
                add_left();
            } else {
                add_forward();
                add_left();
                add_right();
            }

            for (auto [op, nxt] : nexts) {
                int weight = (op == 'F' ? ROUTE_F_COST : ROUTE_TURN_COST);
                int next_dist = cur_dist + weight;
                if (next_dist >= dist[nxt]) continue;
                dist[nxt] = next_dist;
                prev[nxt] = cur;
                prev_op[nxt] = op;
                que.push({next_dist, nxt});
            }
        }
#else
        queue<int> que;

        int start = id_of(nodes[source].r, nodes[source].c, sdir);
        prev[start] = start;
        que.push(start);

        while (!que.empty()) {
            int cur = que.front();
            que.pop();

            int r, c, dir;
            decode(cur, r, c, dir);
            for (int target : nodes_at_cell[r * N + c]) {
                if (best_state[target] == -1) best_state[target] = cur;
            }

            vector<pair<char, int>> nexts;
            auto add_left = [&]() { nexts.push_back({'L', id_of(r, c, (dir + 3) & 3)}); };
            auto add_right = [&]() { nexts.push_back({'R', id_of(r, c, (dir + 1) & 3)}); };
            auto add_forward = [&]() {
                if (can_move(r, c, dir)) {
                    nexts.push_back({'F', id_of(r + dr[dir], c + dc[dir], dir)});
                }
            };

            if (mode == 0) {
                add_left();
                add_right();
                add_forward();
            } else if (mode == 1) {
                add_left();
                add_forward();
                add_right();
            } else if (mode == 2) {
                add_right();
                add_forward();
                add_left();
            } else if (mode == 3) {
                add_right();
                add_left();
                add_forward();
            } else if (mode == 4) {
                add_forward();
                add_right();
                add_left();
            } else {
                add_forward();
                add_left();
                add_right();
            }

            for (auto [op, nxt] : nexts) {
                if (prev[nxt] != -1) continue;
                prev[nxt] = cur;
                prev_op[nxt] = op;
                que.push(nxt);
            }
        }
#endif

        for (int target = 0; target < node_count; ++target) {
            int goal = best_state[target];
            if (goal == -1) continue;

            string ops;
            for (int cur = goal; cur != start; cur = prev[cur]) {
                ops.push_back(prev_op[cur]);
            }
            reverse(ops.begin(), ops.end());

            int er, ec, edir;
            decode(goal, er, ec, edir);
            table[source][sdir][target] = {
                static_cast<int>(ops.size()),
                edir,
                ops,
            };
        }
    };

    for (int source = 0; source < node_count; ++source) {
        for (int sdir = 0; sdir < 4; ++sdir) {
            build_routes(route, tie_mode, source, sdir);
        }
    }

#if ENABLE_ALT_ROUTE_CANDIDATE
    vector<vector<vector<vector<Route>>>> alt_routes(
        6, vector<vector<vector<Route>>>(
               node_count, vector<vector<Route>>(4, vector<Route>(node_count))));
    for (int mode = 0; mode < 6; ++mode) {
        for (int source = 0; source < node_count; ++source) {
            for (int sdir = 0; sdir < 4; ++sdir) {
                build_routes(alt_routes[mode], mode, source, sdir);
            }
        }
    }
#endif

    auto evaluate_order = [&](const vector<int> &order) -> int {
        int cost = 0;
        int current = 0;
        int dir = 1;

        for (int k : order) {
            int ball_node = 1 + k;
            int basket_node = 1 + M + k;
            const Route &to_ball = route[current][dir][ball_node];
            if (to_ball.end_dir == -1) return 1000000000;
            const Route &to_basket = route[ball_node][to_ball.end_dir][basket_node];
            if (to_basket.end_dir == -1) return 1000000000;

            cost += to_ball.cost + 1 + to_basket.cost + 1;
            current = basket_node;
            dir = to_basket.end_dir;
        }

        return cost;
    };

    auto build_answer = [&](const vector<int> &order) -> BuiltAnswer {
        string result;
        int current = 0;
        int dir = 1;
        int delivered = 0;

        for (int k : order) {
            int ball_node = 1 + k;
            int basket_node = 1 + M + k;
            const Route &to_ball = route[current][dir][ball_node];
            if (to_ball.end_dir == -1) break;
            const Route &to_basket = route[ball_node][to_ball.end_dir][basket_node];
            if (to_basket.end_dir == -1) break;

            string segment = to_ball.ops;
            segment.push_back('S');
            segment += to_basket.ops;
            segment.push_back('S');

            if (result.size() + segment.size() > static_cast<size_t>(T)) break;

            result += segment;
            current = basket_node;
            dir = to_basket.end_dir;
            ++delivered;
        }

        return {result, delivered};
    };

#if ENABLE_SCAN_CANDIDATE
    auto build_scan_answer = [&]() -> BuiltAnswer {
        vector<Pos> walk;
        vector<char> used(N * N, 0);
        walk.push_back({0, 0});

        auto dfs_walk = [&](auto &&self, int r, int c) -> void {
            used[r * N + c] = 1;
            const int order_dirs[4] = {1, 2, 3, 0};
            for (int dir : order_dirs) {
                if (!can_move(r, c, dir)) continue;
                int nr = r + dr[dir];
                int nc = c + dc[dir];
                if (used[nr * N + nc]) continue;
                walk.push_back({nr, nc});
                self(self, nr, nc);
                walk.push_back({r, c});
            }
        };
        dfs_walk(dfs_walk, 0, 0);

        string result;
        int dir = 1;
        int delivered = 0;
        vector<char> done(M, 0);

        auto append_turn_to = [&](int ndir, string &ops) {
            int diff = (ndir - dir + 4) & 3;
            if (diff == 1) {
                ops.push_back('R');
                dir = (dir + 1) & 3;
            } else if (diff == 3) {
                ops.push_back('L');
                dir = (dir + 3) & 3;
            } else if (diff == 2) {
                ops.push_back('R');
                ops.push_back('R');
                dir = (dir + 2) & 3;
            }
        };

        auto cell_dir = [&](int r, int c, int nr, int nc) {
            if (nr == r - 1 && nc == c) return 0;
            if (nr == r && nc == c + 1) return 1;
            if (nr == r + 1 && nc == c) return 2;
            return 3;
        };

        vector<int> ball_id_at(N * N, -1), basket_id_at(N * N, -1);
        for (int k = 0; k < M; ++k) {
            ball_id_at[ball[k].r * N + ball[k].c] = k;
            basket_id_at[basket[k].r * N + basket[k].c] = k;
        }

        auto append_scan_pass = [&](const vector<Pos> &path) -> bool {
            const int len = static_cast<int>(path.size());
            vector<vector<int>> occ(N * N);
            for (int i = 0; i < len; ++i) {
                occ[path[i].r * N + path[i].c].push_back(i);
            }

            struct Interval {
                int end;
                int start;
                int id;
            };
            vector<Interval> intervals;
            for (int k = 0; k < M; ++k) {
                if (done[k]) continue;
                int bcell = ball[k].r * N + ball[k].c;
                int ccell = basket[k].r * N + basket[k].c;
                const vector<int> &bs = occ[bcell];
                const vector<int> &cs = occ[ccell];
                int best_start = -1;
                int best_end = 1000000000;
                for (int s : bs) {
                    auto it = upper_bound(cs.begin(), cs.end(), s);
                    if (it == cs.end()) continue;
                    int e = *it;
                    if (e < best_end) {
                        best_end = e;
                        best_start = s;
                    }
                }
                if (best_start != -1) intervals.push_back({best_end, best_start, k});
            }

            sort(intervals.begin(), intervals.end(), [](const Interval &a, const Interval &b) {
                if (a.end != b.end) return a.end < b.end;
                return a.start < b.start;
            });

            vector<int> event(len, -1);
            int last_end = -1;
            int picked = 0;
            for (const Interval &interval : intervals) {
                if (interval.start <= last_end) continue;
                if (event[interval.start] != -1 || event[interval.end] != -1) continue;
                event[interval.start] = interval.id;
                event[interval.end] = interval.id;
                last_end = interval.end;
                ++picked;
            }
            if (picked == 0) return false;

            string ops;
            ops.reserve(len * 3 + 2 * picked);
            int cr = path[0].r;
            int cc = path[0].c;
            for (int i = 0; i < len; ++i) {
                if (event[i] != -1) ops.push_back('S');
                if (i + 1 == len) break;
                int nr = path[i + 1].r;
                int nc = path[i + 1].c;
                int ndir = cell_dir(cr, cc, nr, nc);
                append_turn_to(ndir, ops);
                ops.push_back('F');
                cr = nr;
                cc = nc;
            }

            if (result.size() + ops.size() > static_cast<size_t>(T)) return false;
            result += ops;
            for (const Interval &interval : intervals) {
                if (event[interval.start] == interval.id && event[interval.end] == interval.id) {
                    if (!done[interval.id]) {
                        done[interval.id] = 1;
                        ++delivered;
                    }
                }
            }
            return true;
        };

        vector<Pos> reversed_walk = walk;
        reverse(reversed_walk.begin(), reversed_walk.end());

        const int max_passes = max(2, M + 2);
        for (int pass = 0; pass < max_passes && delivered < M; ++pass) {
            const vector<Pos> &path = (pass & 1) ? reversed_walk : walk;
            if (!append_scan_pass(path)) break;
        }

        return {result, delivered};
    };
#endif

    vector<int> order;
    vector<bool> done(M, false);
    int current = 0;
    int dir = 1;

    for (int delivered = 0; delivered < M; ++delivered) {
        int best_k = -1;
        int best_cost = 1000000000;
        int best_end_dir = -1;

        for (int k = 0; k < M; ++k) {
            if (done[k]) continue;

            int ball_node = 1 + k;
            int basket_node = 1 + M + k;
            const Route &to_ball = route[current][dir][ball_node];
            if (to_ball.end_dir == -1) continue;
            const Route &to_basket = route[ball_node][to_ball.end_dir][basket_node];
            if (to_basket.end_dir == -1) continue;

            int cost = to_ball.cost + 1 + to_basket.cost + 1;
            if (cost >= best_cost) continue;

            best_k = k;
            best_cost = cost;
            best_end_dir = to_basket.end_dir;
        }

        if (best_k == -1) break;

        order.push_back(best_k);
        done[best_k] = true;
        current = 1 + M + best_k;
        dir = best_end_dir;
    }

    auto improve_order = [&](vector<int> base_order, int max_rounds) -> vector<int> {
        int current_cost = evaluate_order(base_order);
        bool improved = true;
        for (int round = 0; improved && round < max_rounds; ++round) {
            if (order_time_over()) break;
            improved = false;
            vector<int> best_order = base_order;
            int best_cost = current_cost;
            const int n = static_cast<int>(base_order.size());
            int checks = 0;

            for (int i = 0; i < n; ++i) {
                for (int j = i + 1; j < n; ++j) {
                    vector<int> candidate = base_order;
                    swap(candidate[i], candidate[j]);
                    int cost = evaluate_order(candidate);
                    if ((++checks & 255) == 0 && order_time_over()) {
                        return best_cost < current_cost ? best_order : base_order;
                    }
                    if (cost < best_cost) {
                        best_cost = cost;
                        best_order = candidate;
                    }
                }
            }

            for (int i = 0; i < n; ++i) {
                for (int j = i + 1; j < n; ++j) {
                    vector<int> candidate = base_order;
                    reverse(candidate.begin() + i, candidate.begin() + j + 1);
                    int cost = evaluate_order(candidate);
                    if ((++checks & 255) == 0 && order_time_over()) {
                        return best_cost < current_cost ? best_order : base_order;
                    }
                    if (cost < best_cost) {
                        best_cost = cost;
                        best_order = candidate;
                    }
                }
            }

            for (int i = 0; i < n; ++i) {
                for (int j = 0; j <= n; ++j) {
                    if (j == i || j == i + 1) continue;
                    vector<int> candidate = base_order;
                    int value = candidate[i];
                    candidate.erase(candidate.begin() + i);
                    int insert_pos = j;
                    if (i < j) --insert_pos;
                    candidate.insert(candidate.begin() + insert_pos, value);
                    int cost = evaluate_order(candidate);
                    if ((++checks & 255) == 0 && order_time_over()) {
                        return best_cost < current_cost ? best_order : base_order;
                    }
                    if (cost < best_cost) {
                        best_cost = cost;
                        best_order = candidate;
                    }
                }
            }

            if (best_cost < current_cost) {
                base_order = best_order;
                current_cost = best_cost;
                improved = true;
            }
        }

        return base_order;
    };

    auto make_beam_orders = [&]() -> vector<vector<int>> {
        struct BeamState {
            unsigned long long mask;
            int current;
            int dir;
            int cost;
            vector<int> order;
        };

        const int beam_width = light_order_mode ? min(ORDER_BEAM_WIDTH, 20) : ORDER_BEAM_WIDTH;
        vector<BeamState> beam;
        beam.push_back({0ULL, 0, 1, 0, {}});

        for (int step = 0; step < M; ++step) {
            vector<BeamState> next;
            next.reserve(beam.size() * max(1, M - step));

            for (const BeamState &state : beam) {
                for (int k = 0; k < M; ++k) {
                    if ((state.mask >> k) & 1ULL) continue;

                    int ball_node = 1 + k;
                    int basket_node = 1 + M + k;
                    const Route &to_ball = route[state.current][state.dir][ball_node];
                    if (to_ball.end_dir == -1) continue;
                    const Route &to_basket = route[ball_node][to_ball.end_dir][basket_node];
                    if (to_basket.end_dir == -1) continue;

                    BeamState candidate = state;
                    candidate.mask |= 1ULL << k;
                    candidate.current = basket_node;
                    candidate.dir = to_basket.end_dir;
                    candidate.cost += to_ball.cost + 1 + to_basket.cost + 1;
                    candidate.order.push_back(k);
                    next.push_back(candidate);
                }
            }

            sort(next.begin(), next.end(), [](const BeamState &a, const BeamState &b) {
                return a.cost < b.cost;
            });
            if (static_cast<int>(next.size()) > beam_width) next.resize(beam_width);
            beam = next;
            if (beam.empty()) break;
        }

        vector<vector<int>> orders;
        const int keep_limit = light_order_mode ? min(ORDER_BEAM_KEEP, 10) : ORDER_BEAM_KEEP;
        const int keep = min(keep_limit, static_cast<int>(beam.size()));
        for (int i = 0; i < keep; ++i) orders.push_back(beam[i].order);
        return orders;
    };

    auto encode_macro_segment = [&](const string &base, int begin, int len, int end) -> string {
        string result;
        string pattern = base.substr(begin, len);
        result.push_back('M');
        result += pattern;
        result.push_back('M');

        int pos = begin + len;
        while (pos < end) {
            if (pos + len <= end && base.compare(pos, len, pattern) == 0) {
                result.push_back('P');
                pos += len;
            } else {
                result.push_back(base[pos]);
                ++pos;
            }
        }

        return result;
    };

    auto compress_dp = [&](const string &base) -> string {
        const int n = static_cast<int>(base.size());
        const int inf = 1000000000;
        vector<int> dp(n + 1, inf);
        vector<PrevStep> prev(n + 1);
        dp[0] = 0;

        for (int i = 0; i < n; ++i) {
            if (dp[i] == inf) continue;

            if (dp[i] + 1 < dp[i + 1]) {
                dp[i + 1] = dp[i] + 1;
                prev[i + 1] = {i, 0, i + 1, false};
            }

            const int max_len = min(42, n - i);
            for (int len = 2; len <= max_len; ++len) {
                string pattern = base.substr(i, len);
                int count = 1;
                int last_end = i + len;
                size_t search_pos = static_cast<size_t>(last_end);

                while (search_pos < static_cast<size_t>(n)) {
                    size_t found = base.find(pattern, search_pos);
                    if (found == string::npos) break;

                    ++count;
                    last_end = static_cast<int>(found) + len;
                    search_pos = static_cast<size_t>(last_end);

                    int saving = (count - 1) * (len - 1) - 2;
                    if (saving <= 0) continue;

                    int encoded_cost = (last_end - i) - saving;
                    if (dp[i] + encoded_cost < dp[last_end]) {
                        dp[last_end] = dp[i] + encoded_cost;
                        prev[last_end] = {i, len, last_end, true};
                    }
                }
            }
        }

        if (dp[n] >= n) return base;

        vector<PrevStep> steps;
        for (int pos = n; pos > 0; pos = prev[pos].prev) {
            steps.push_back(prev[pos]);
        }
        reverse(steps.begin(), steps.end());

        string compressed;
        compressed.reserve(dp[n]);
        for (const PrevStep &step : steps) {
            if (step.macro) {
                compressed += encode_macro_segment(base, step.prev, step.len, step.end);
            } else {
                compressed.push_back(base[step.prev]);
            }
        }

        if (compressed.size() < base.size()) return compressed;
        return base;
    };

    auto compress_output = [&](const string &base) -> string {
        return compress_dp(base);
    };

    auto compress_hierarchical = [&](const string &base, const string &fallback) -> string {
        const int n = static_cast<int>(base.size());
        if (n == 0 || time_over()) return fallback;

        struct MacroCand {
            string pattern;
            int gain;
        };

        vector<MacroCand> cands;
        unordered_map<string, int> best_gain;
        for (int len = 2; len <= min(HIER_MAX_LEN, n); ++len) {
            unordered_map<string, int> count;
            count.reserve(n);
            for (int i = 0; i + len <= n; ++i) {
                ++count[base.substr(i, len)];
            }
            for (const auto &[pattern, cnt] : count) {
                if (cnt < 2) continue;
                int gain = (cnt - 1) * (len - 1) - 2;
                if (gain <= 0) continue;
                auto it = best_gain.find(pattern);
                if (it == best_gain.end() || gain > it->second) best_gain[pattern] = gain;
            }
            if ((len & 7) == 0 && time_over()) return fallback;
        }
        cands.reserve(best_gain.size());
        for (const auto &[pattern, gain] : best_gain) cands.push_back({pattern, gain});
        sort(cands.begin(), cands.end(), [](const MacroCand &a, const MacroCand &b) {
            if (a.gain != b.gain) return a.gain > b.gain;
            return a.pattern.size() > b.pattern.size();
        });
        if (static_cast<int>(cands.size()) > HIER_CAND_LIMIT) cands.resize(HIER_CAND_LIMIT);

        vector<string> macros(1, "");
        for (const MacroCand &cand : cands) macros.push_back(cand.pattern);

        vector<vector<int>> starts(n + 1);
        for (int id = 1; id < static_cast<int>(macros.size()); ++id) {
            const string &pattern = macros[id];
            size_t pos = base.find(pattern);
            while (pos != string::npos) {
                starts[static_cast<int>(pos)].push_back(id);
                pos = base.find(pattern, pos + 1);
            }
        }
        for (vector<int> &ids : starts) {
            sort(ids.begin(), ids.end(), [&](int a, int b) {
                return macros[a].size() > macros[b].size();
            });
            if (static_cast<int>(ids.size()) > HIER_START_LIMIT) ids.resize(HIER_START_LIMIT);
        }

        auto encode_with_old = [&](const string &pattern, const string &old_macro) {
            if (old_macro.empty()) return pattern;
            string encoded;
            int i = 0;
            const int old_len = static_cast<int>(old_macro.size());
            while (i < static_cast<int>(pattern.size())) {
                if (i + old_len <= static_cast<int>(pattern.size()) &&
                    pattern.compare(i, old_len, old_macro) == 0) {
                    encoded.push_back('P');
                    i += old_len;
                } else {
                    encoded.push_back(pattern[i]);
                    ++i;
                }
            }
            return encoded;
        };

        const int macro_count = static_cast<int>(macros.size());
        vector<vector<unsigned short>> encoded_cost(
            macro_count, vector<unsigned short>(macro_count, 0));
        for (int old_id = 0; old_id < macro_count; ++old_id) {
            const string &old_macro = macros[old_id];
            int old_len = static_cast<int>(old_macro.size());
            for (int next_id = 1; next_id < macro_count; ++next_id) {
                const string &pattern = macros[next_id];
                if (old_len == 0) {
                    encoded_cost[old_id][next_id] =
                        static_cast<unsigned short>(pattern.size());
                    continue;
                }

                int encoded_len = 0;
                int i = 0;
                while (i < static_cast<int>(pattern.size())) {
                    if (i + old_len <= static_cast<int>(pattern.size()) &&
                        pattern.compare(i, old_len, old_macro) == 0) {
                        ++encoded_len;
                        i += old_len;
                    } else {
                        ++encoded_len;
                        ++i;
                    }
                }
                encoded_cost[old_id][next_id] =
                    static_cast<unsigned short>(encoded_len);
            }
        };

        struct HPrev {
            int prev_pos = -1;
            short prev_macro = -1;
            char action = 0;
        };

        const int inf = 1000000000;
        const int state_count = (n + 1) * macro_count;
        vector<int> dp(state_count, inf);
        vector<HPrev> prev(state_count);
        auto state_index = [&](int pos, int macro_id) {
            return pos * macro_count + macro_id;
        };

        dp[state_index(0, 0)] = 0;

        for (int pos = 0; pos <= n; ++pos) {
            if ((pos & 7) == 0 && time_over()) return fallback;
            for (int macro_id = 0; macro_id < macro_count; ++macro_id) {
                int cur_index = state_index(pos, macro_id);
                int cur_cost = dp[cur_index];
                if (cur_cost == inf) continue;

                if (pos < n) {
                    int next_index = state_index(pos + 1, macro_id);
                    if (cur_cost + 1 < dp[next_index]) {
                        dp[next_index] = cur_cost + 1;
                        prev[next_index] = {pos, static_cast<short>(macro_id), 'L'};
                    }
                }

                const string &old_macro = macros[macro_id];
                int old_len = static_cast<int>(old_macro.size());
                if (old_len > 0 && pos + old_len <= n &&
                    base.compare(pos, old_len, old_macro) == 0) {
                    int next_index = state_index(pos + old_len, macro_id);
                    if (cur_cost + 1 < dp[next_index]) {
                        dp[next_index] = cur_cost + 1;
                        prev[next_index] = {pos, static_cast<short>(macro_id), 'P'};
                    }
                }

                if (pos == n) continue;
                for (int next_id : starts[pos]) {
                    const string &pattern = macros[next_id];
                    int pat_len = static_cast<int>(pattern.size());
                    int next_pos = pos + pat_len;
                    int next_cost = cur_cost + 2 + encoded_cost[macro_id][next_id];
                    int next_index = state_index(next_pos, next_id);
                    if (next_cost < dp[next_index]) {
                        dp[next_index] = next_cost;
                        prev[next_index] = {pos, static_cast<short>(macro_id), 'M'};
                    }
                }
            }
        }

        int best_macro = 0;
        int best_cost = inf;
        for (int macro_id = 0; macro_id < macro_count; ++macro_id) {
            int cost = dp[state_index(n, macro_id)];
            if (cost < best_cost) {
                best_cost = cost;
                best_macro = macro_id;
            }
        }

        if (best_cost >= static_cast<int>(fallback.size())) return fallback;

        vector<string> chunks;
        chunks.reserve(n);
        int pos = n;
        int macro_id = best_macro;
        while (pos > 0) {
            HPrev step = prev[state_index(pos, macro_id)];
            if (step.action == 0) return fallback;

            if (step.action == 'L') {
                chunks.emplace_back(1, base[step.prev_pos]);
            } else if (step.action == 'P') {
                chunks.emplace_back("P");
            } else {
                string encoded = encode_with_old(macros[macro_id], macros[step.prev_macro]);
                string chunk;
                chunk.reserve(encoded.size() + 2);
                chunk.push_back('M');
                chunk += encoded;
                chunk.push_back('M');
                chunks.push_back(std::move(chunk));
            }

            pos = step.prev_pos;
            macro_id = step.prev_macro;
        }

        string best_output;
        best_output.reserve(best_cost);
        for (auto it = chunks.rbegin(); it != chunks.rend(); ++it) {
            best_output += *it;
        }
        if (best_output.size() < fallback.size()) return best_output;
        return fallback;
    };

    auto score_tuple = [&](const BuiltAnswer &built, const string &candidate_output) {
        int buttons = static_cast<int>(candidate_output.size());
        int basic = static_cast<int>(built.base.size());
        long long absolute_score =
            (built.delivered == M ? buttons : 1LL * T * (M - built.delivered));
        return make_tuple(absolute_score, buttons, basic);
    };

    vector<pair<vector<int>, int>> candidates;
    auto add_candidate = [&](vector<int> candidate, int rounds) {
        if (candidate.empty()) return;
        candidates.push_back({std::move(candidate), rounds});
    };

    add_candidate(order, IMPROVE_FIRST_ROUNDS);
    if (!macro_priority_mode) {
        for (const vector<int> &beam_order : make_beam_orders()) {
            add_candidate(beam_order, IMPROVE_CANDIDATE_ROUNDS);
        }
    }

#if ENABLE_PATTERN_ORDERS
    vector<int> ids(M);
    for (int i = 0; i < M; ++i) ids[i] = i;

    auto snake_key = [&](const Pos &p) {
        return p.r * N + ((p.r & 1) ? (N - 1 - p.c) : p.c);
    };
    auto add_sorted_order = [&](auto key_func, int rounds) {
        vector<int> candidate = ids;
        stable_sort(candidate.begin(), candidate.end(), [&](int a, int b) {
            auto ka = key_func(a);
            auto kb = key_func(b);
            if (ka != kb) return ka < kb;
            return a < b;
        });
        add_candidate(candidate, rounds);
    };

    add_sorted_order([&](int k) {
        return make_tuple(snake_key(ball[k]), snake_key(basket[k]));
    }, 0);
    add_sorted_order([&](int k) {
        return make_tuple(snake_key(basket[k]), snake_key(ball[k]));
    }, 0);
    add_sorted_order([&](int k) {
        int mr = ball[k].r + basket[k].r;
        int mc = ball[k].c + basket[k].c;
        return make_tuple(mr / 2, mc / 2, snake_key(ball[k]));
    }, 0);
    add_sorted_order([&](int k) {
        int dr0 = basket[k].r - ball[k].r;
        int dc0 = basket[k].c - ball[k].c;
        int br = (dr0 > 0) - (dr0 < 0);
        int bc = (dc0 > 0) - (dc0 < 0);
        return make_tuple(br, bc, abs(dr0) + abs(dc0), snake_key(ball[k]));
    }, 0);
    add_sorted_order([&](int k) {
        int dr0 = basket[k].r - ball[k].r;
        int dc0 = basket[k].c - ball[k].c;
        int br = (dr0 > 0) - (dr0 < 0);
        int bc = (dc0 > 0) - (dc0 < 0);
        return make_tuple(br, bc, snake_key(basket[k]), snake_key(ball[k]));
    }, 0);
#endif

#if ENABLE_CARRY_PATTERN_ORDERS
    {
        vector<int> ids(M);
        for (int i = 0; i < M; ++i) ids[i] = i;

        auto snake_key = [&](const Pos &p) {
            return p.r * N + ((p.r & 1) ? (N - 1 - p.c) : p.c);
        };

        vector<string> carry_sig(M);
        vector<int> carry_len(M, 1000000000);
        for (int k = 0; k < M; ++k) {
            int ball_node = 1 + k;
            int basket_node = 1 + M + k;
            for (int sdir = 0; sdir < 4; ++sdir) {
                const Route &r = route[ball_node][sdir][basket_node];
                if (r.end_dir == -1) continue;
                if (r.cost < carry_len[k] ||
                    (r.cost == carry_len[k] && r.ops < carry_sig[k])) {
                    carry_len[k] = r.cost;
                    carry_sig[k] = r.ops;
                }
            }
        }

        unordered_map<string, int> sig_count;
        sig_count.reserve(M * 2 + 1);
        for (const string &sig : carry_sig) {
            if (!sig.empty()) ++sig_count[sig];
        }

        vector<int> repeated_first = order;
        stable_sort(repeated_first.begin(), repeated_first.end(), [&](int a, int b) {
            int ca = sig_count[carry_sig[a]];
            int cb = sig_count[carry_sig[b]];
            bool ra = ca >= 2;
            bool rb = cb >= 2;
            if (ra != rb) return ra > rb;
            if (ra && carry_sig[a] != carry_sig[b]) return carry_sig[a] < carry_sig[b];
            if (carry_len[a] != carry_len[b]) return carry_len[a] < carry_len[b];
            return snake_key(ball[a]) < snake_key(ball[b]);
        });
        add_candidate(repeated_first, 0);

        vector<int> signature_sorted = ids;
        stable_sort(signature_sorted.begin(), signature_sorted.end(), [&](int a, int b) {
            int ca = sig_count[carry_sig[a]];
            int cb = sig_count[carry_sig[b]];
            bool ra = ca >= 2;
            bool rb = cb >= 2;
            if (ra != rb) return ra > rb;
            if (carry_sig[a] != carry_sig[b]) return carry_sig[a] < carry_sig[b];
            if (carry_len[a] != carry_len[b]) return carry_len[a] < carry_len[b];
            int ma = snake_key({(ball[a].r + basket[a].r) / 2, (ball[a].c + basket[a].c) / 2});
            int mb = snake_key({(ball[b].r + basket[b].r) / 2, (ball[b].c + basket[b].c) / 2});
            if (ma != mb) return ma < mb;
            return a < b;
        });
        add_candidate(signature_sorted, 0);
    }
#endif

    string output;
    string best_base;
    auto best_score = make_tuple(1LL << 60, 1000000000, 1000000000);
    vector<int> best_order;

    for (int idx = 0; idx < static_cast<int>(candidates.size()); ++idx) {
        if (idx > 0 && order_time_over()) break;
        vector<int> candidate_order = candidates[idx].first;
        int improve_rounds = candidates[idx].second;
        if (light_order_mode) {
            improve_rounds = min(improve_rounds, idx == 0 ? 50 : 4);
        }
        candidate_order = improve_order(candidate_order, improve_rounds);
        BuiltAnswer answer = build_answer(candidate_order);
        string candidate_output = compress_output(answer.base);

        auto candidate_score = score_tuple(answer, candidate_output);
        if (candidate_score < best_score) {
            best_score = candidate_score;
            output = candidate_output;
            best_base = answer.base;
            best_order = candidate_order;
        }
    }

    if (output.empty()) {
        BuiltAnswer answer = build_answer(improve_order(order, IMPROVE_FIRST_ROUNDS));
        output = compress_output(answer.base);
        best_base = answer.base;
    }

#if ENABLE_SCAN_CANDIDATE
    if (!time_over()) {
        BuiltAnswer scan_answer = build_scan_answer();
        string scan_output = compress_output(scan_answer.base);
        if (!time_over()) {
            scan_output = compress_hierarchical(scan_answer.base, scan_output);
        }
        auto scan_score = score_tuple(scan_answer, scan_output);
        if (scan_score < best_score) {
            best_score = scan_score;
            output = scan_output;
            best_base = scan_answer.base;
        }
    }
#endif

    if (!best_base.empty() && !time_over()) {
        output = compress_hierarchical(best_base, output);
    }

    for (char op : output) {
        cout << op << '\n';
    }

    return 0;
}
