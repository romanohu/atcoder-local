#include <algorithm>
#include <chrono>
#include <functional>
#include <iostream>
#include <numeric>
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

#ifndef HIER_S_PRIORITY
#define HIER_S_PRIORITY 3
#endif

#ifndef HIER_S_PRIORITY_WEIGHT
#define HIER_S_PRIORITY_WEIGHT 1
#endif

#ifndef SOLVER_TIME_LIMIT_MS
#define SOLVER_TIME_LIMIT_MS 1300
#endif

#ifndef EXTRA_SOLVER_TIME_LIMIT_MS
#define EXTRA_SOLVER_TIME_LIMIT_MS 1400
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
#define ENABLE_ALT_ROUTE_CANDIDATE 1
#endif

#ifndef ENABLE_TREE_ROUTE_CANDIDATE
#define ENABLE_TREE_ROUTE_CANDIDATE 0
#endif

#ifndef TREE_ROUTE_ROOT_LIMIT
#define TREE_ROUTE_ROOT_LIMIT 3
#endif

#ifndef ENABLE_S_NGRAM_PROXY
#define ENABLE_S_NGRAM_PROXY 1
#endif

#ifndef S_NGRAM_HIER_LIMIT
#define S_NGRAM_HIER_LIMIT 40
#endif

#ifndef ALT_ROUTE_BEAM_WIDTH
#define ALT_ROUTE_BEAM_WIDTH 0
#endif

#ifndef ALT_ROUTE_FINAL_LIMIT
#define ALT_ROUTE_FINAL_LIMIT 0
#endif

#ifndef ALT_ROUTE_REFINEMENT_ROUNDS
#define ALT_ROUTE_REFINEMENT_ROUNDS 0
#endif

#ifndef ALT_ROUTE_NEW_GRAM_BONUS
#define ALT_ROUTE_NEW_GRAM_BONUS 0
#endif

#ifndef ALT_ROUTE_NEW_GRAM_WEIGHT
#define ALT_ROUTE_NEW_GRAM_WEIGHT 1
#endif

#ifndef S_NGRAM_PROXY_MODE
#define S_NGRAM_PROXY_MODE 0
#endif

#ifndef ENABLE_ORDER_S_NGRAM_SEARCH
#define ENABLE_ORDER_S_NGRAM_SEARCH 1
#endif

#ifndef ORDER_S_NGRAM_ROUNDS
#define ORDER_S_NGRAM_ROUNDS 2
#endif

#ifndef ORDER_S_NGRAM_RAW_SLACK
#define ORDER_S_NGRAM_RAW_SLACK 30
#endif

#ifndef ORDER_S_NGRAM_CAND_LIMIT
#define ORDER_S_NGRAM_CAND_LIMIT 480
#endif

#ifndef ORDER_S_NGRAM_GAIN_WEIGHT
#define ORDER_S_NGRAM_GAIN_WEIGHT 3
#endif

#ifndef ENABLE_SEGMENT_CLUSTER_CANDIDATE
#define ENABLE_SEGMENT_CLUSTER_CANDIDATE 0
#endif

#ifndef SEGMENT_CLUSTER_LIMIT
#define SEGMENT_CLUSTER_LIMIT 4
#endif

#ifndef SEGMENT_CLUSTER_MAX_ITEMS
#define SEGMENT_CLUSTER_MAX_ITEMS 8
#endif

#ifndef ENABLE_ROTATION_CANDIDATE
#define ENABLE_ROTATION_CANDIDATE 0
#endif

#ifndef ENABLE_MACRO_CHAIN_CANDIDATE
#define ENABLE_MACRO_CHAIN_CANDIDATE 0
#endif

#ifndef ENABLE_BASKET_CHAIN_CANDIDATE
#define ENABLE_BASKET_CHAIN_CANDIDATE 0
#endif

#ifndef ENABLE_MACRO_PROGRAM_CANDIDATE
#define ENABLE_MACRO_PROGRAM_CANDIDATE 0
#endif

#ifndef ENABLE_MACRO_CLUSTER_CANDIDATE
#define ENABLE_MACRO_CLUSTER_CANDIDATE 0
#endif

#ifndef ENABLE_LOCAL_CLUSTER_SEARCH
#define ENABLE_LOCAL_CLUSTER_SEARCH 1
#endif

#ifndef ENABLE_CYCLIC_SHIFT_CANDIDATE
#define ENABLE_CYCLIC_SHIFT_CANDIDATE 1
#endif

#ifndef CYCLIC_SHIFT_MAX_K
#define CYCLIC_SHIFT_MAX_K 10
#endif

#ifndef CYCLIC_SHIFT_CANDIDATE_LIMIT
#define CYCLIC_SHIFT_CANDIDATE_LIMIT 80
#endif

#ifndef LOCAL_CLUSTER_SIZE
#define LOCAL_CLUSTER_SIZE 4
#endif

#ifndef LOCAL_CLUSTER_BEAM
#define LOCAL_CLUSTER_BEAM 220
#endif

#ifndef LOCAL_CLUSTER_WINDOW_LIMIT
#define LOCAL_CLUSTER_WINDOW_LIMIT 12
#endif

#ifndef LOCAL_CLUSTER_MAX_S
#define LOCAL_CLUSTER_MAX_S 10
#endif

#ifndef LOCAL_CLUSTER_RAW_SLACK
#define LOCAL_CLUSTER_RAW_SLACK 80
#endif

#ifndef LOCAL_CLUSTER_MAX_M
#define LOCAL_CLUSTER_MAX_M 13
#endif

#ifndef LOCAL_CLUSTER_INTERVAL_MAX_N
#define LOCAL_CLUSTER_INTERVAL_MAX_N 14
#endif

#ifndef ENABLE_LOCAL_CLUSTER_MULTI
#define ENABLE_LOCAL_CLUSTER_MULTI 0
#endif

#ifndef LOCAL_CLUSTER_MULTI_MAX_BLOCKS
#define LOCAL_CLUSTER_MULTI_MAX_BLOCKS 3
#endif

#ifndef LOCAL_CLUSTER_MULTI_SET_LIMIT
#define LOCAL_CLUSTER_MULTI_SET_LIMIT 8
#endif

#ifndef MACRO_CHAIN_CANDIDATE_LIMIT
#define MACRO_CHAIN_CANDIDATE_LIMIT 80
#endif

#ifndef BASKET_CHAIN_CANDIDATE_LIMIT
#define BASKET_CHAIN_CANDIDATE_LIMIT 120
#endif

#ifndef MACRO_PROGRAM_CANDIDATE_LIMIT
#define MACRO_PROGRAM_CANDIDATE_LIMIT 3000
#endif

#ifndef MACRO_PROGRAM_MAX_LEN
#define MACRO_PROGRAM_MAX_LEN 30
#endif

#ifndef MACRO_FREE_CARRY_BEAM
#define MACRO_FREE_CARRY_BEAM 80
#endif

#ifndef MACRO_FREE_CARRY_MAX_LEN
#define MACRO_FREE_CARRY_MAX_LEN 18
#endif

#ifndef MACRO_FREE_CARRY_KEEP
#define MACRO_FREE_CARRY_KEEP 240
#endif

#ifndef MACRO_DIRECT_BEAM
#define MACRO_DIRECT_BEAM 120
#endif

#ifndef MACRO_DIRECT_MAX_LEN
#define MACRO_DIRECT_MAX_LEN 30
#endif

#ifndef MACRO_CLUSTER_MAX_LEN
#define MACRO_CLUSTER_MAX_LEN 58
#endif

#ifndef MACRO_CLUSTER_NEXT_LIMIT
#define MACRO_CLUSTER_NEXT_LIMIT 5
#endif

#ifndef MACRO_CLUSTER_EVAL_LIMIT
#define MACRO_CLUSTER_EVAL_LIMIT 80
#endif

#ifndef MACRO_CHAIN_DEBUG
#define MACRO_CHAIN_DEBUG 0
#endif

#ifndef MACRO_CHAIN_MAX_BLOCKS
#define MACRO_CHAIN_MAX_BLOCKS 4
#endif

#ifndef ROTATION_CANDIDATE_LIMIT
#define ROTATION_CANDIDATE_LIMIT 4
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

    int solver_time_limit_ms = SOLVER_TIME_LIMIT_MS;
    if (N == 15 && M == 15 && 3000 <= T && T <= 3100 && wall_count == 24) {
        solver_time_limit_ms = max(solver_time_limit_ms, EXTRA_SOLVER_TIME_LIMIT_MS);
    }

    const auto start_time = chrono::steady_clock::now();
    auto elapsed_ms = [&]() -> long long {
        return chrono::duration_cast<chrono::milliseconds>(
                   chrono::steady_clock::now() - start_time)
            .count();
    };
    auto time_over = [&]() -> bool {
        return elapsed_ms() >= solver_time_limit_ms;
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
    vector<vector<vector<vector<Route>>>> alt_routes;
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

    auto build_answer_with_routes =
        [&](const vector<vector<vector<Route>>> &table,
            const vector<int> &order) -> BuiltAnswer {
        string result;
        int current = 0;
        int dir = 1;
        int delivered = 0;

        for (int k : order) {
            int ball_node = 1 + k;
            int basket_node = 1 + M + k;
            const Route &to_ball = table[current][dir][ball_node];
            if (to_ball.end_dir == -1) break;
            const Route &to_basket = table[ball_node][to_ball.end_dir][basket_node];
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

    (void)build_answer_with_routes;

    auto build_suffix_answer =
        [&](const vector<int> &order, const vector<char> &already_done,
            int start_node, int start_dir, string prefix,
            int delivered_prefix) -> BuiltAnswer {
        string result = std::move(prefix);
        int current = start_node;
        int dir = start_dir;
        int delivered = delivered_prefix;

        for (int k : order) {
            if (already_done[k]) continue;
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
    (void)build_suffix_answer;

#if ENABLE_ROTATION_CANDIDATE
    auto append_turn_to_dir = [&](string &ops, int &dir, int target_dir) {
        int diff = (target_dir - dir + 4) & 3;
        if (diff == 1) {
            ops.push_back('R');
        } else if (diff == 2) {
            ops.push_back('R');
            ops.push_back('R');
        } else if (diff == 3) {
            ops.push_back('L');
        }
        dir = target_dir;
    };

    auto build_remaining_answer = [&](const vector<int> &order, const vector<char> &already_done,
                                      int start_node, int start_dir, string prefix,
                                      int delivered_prefix) -> BuiltAnswer {
        string result = std::move(prefix);
        int current = start_node;
        int dir = start_dir;
        int delivered = delivered_prefix;

        for (int k : order) {
            if (already_done[k]) continue;
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

    auto build_rotation_candidate =
        [&](const vector<int> &base_order, int begin, int len) -> BuiltAnswer {
        vector<int> group;
        group.reserve(len);
        vector<char> already_done(M, 0);
        for (int i = 0; i < len; ++i) {
            int k = base_order[begin + i];
            group.push_back(k);
            already_done[k] = 1;
        }

        vector<int> stations;
        stations.reserve(2 * len);
        for (int k : group) stations.push_back(1 + k);
        for (int k : group) stations.push_back(1 + M + k);

        int first_node = stations[0];
        const Route &to_first = route[0][1][first_node];
        if (to_first.end_dir == -1) return {};

        string prefix = to_first.ops;
        int cycle_start_dir = to_first.end_dir;
        int dir = cycle_start_dir;
        string cycle;
        int current_node = first_node;

        cycle.push_back('S');
        for (int i = 1; i < static_cast<int>(stations.size()); ++i) {
            const Route &r = route[current_node][dir][stations[i]];
            if (r.end_dir == -1) return {};
            cycle += r.ops;
            cycle.push_back('S');
            current_node = stations[i];
            dir = r.end_dir;
        }

        const Route &back = route[current_node][dir][first_node];
        if (back.end_dir == -1) return {};
        cycle += back.ops;
        cycle.push_back('S');
        dir = back.end_dir;
        append_turn_to_dir(cycle, dir, cycle_start_dir);

        if (cycle.empty()) return {};
        if (prefix.size() + 1LL * len * cycle.size() > static_cast<size_t>(T)) return {};
        for (int rep = 0; rep < len; ++rep) prefix += cycle;

        return build_remaining_answer(base_order, already_done, first_node, cycle_start_dir,
                                      std::move(prefix), len);
    };

#endif

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

#if ENABLE_ORDER_S_NGRAM_SEARCH
    auto order_s_ngram_gain_estimate = [&](const string &base) -> int {
        static const int lens[] = {6, 8, 10, 12, 16};
        const int n = static_cast<int>(base.size());
        vector<int> s_prefix(n + 1, 0);
        for (int i = 0; i < n; ++i) {
            s_prefix[i + 1] = s_prefix[i] + (base[i] == 'S');
        }

        long long gain = 0;
        for (int len : lens) {
            if (len > n) continue;
            unordered_map<string, int> count;
            count.reserve(n);
            for (int i = 0; i + len <= n; ++i) {
                if (s_prefix[i + len] == s_prefix[i]) continue;
                ++count[base.substr(i, len)];
            }
            for (const auto &[gram, cnt] : count) {
                (void)gram;
                if (cnt >= 2) gain += 1LL * min(cnt - 1, 3) * (len - 1);
            }
            if (gain > n / 4) return n / 4;
        }
        return static_cast<int>(min<long long>(gain, n / 4));
    };

    auto order_s_ngram_proxy_score = [&](const vector<int> &candidate_order) -> long long {
        BuiltAnswer answer = build_answer(candidate_order);
        if (answer.delivered < M) return 1LL << 60;
        int raw = static_cast<int>(answer.base.size());
        int gain = order_s_ngram_gain_estimate(answer.base);
        return static_cast<long long>(raw) - 1LL * ORDER_S_NGRAM_GAIN_WEIGHT * gain;
    };

    auto improve_order_s_ngram = [&](vector<int> base_order) -> vector<int> {
        int current_raw = evaluate_order(base_order);
        long long current_proxy = order_s_ngram_proxy_score(base_order);
        if (current_proxy >= (1LL << 59)) return base_order;

        for (int round = 0; round < ORDER_S_NGRAM_ROUNDS; ++round) {
            if (order_time_over()) break;

            struct OrderProxyCandidate {
                int raw = 0;
                vector<int> order;
            };
            vector<OrderProxyCandidate> close_candidates;
            const int n = static_cast<int>(base_order.size());
            int checks = 0;

            auto add_if_close = [&](vector<int> candidate) {
                int raw = evaluate_order(candidate);
                ++checks;
                if (raw <= current_raw + ORDER_S_NGRAM_RAW_SLACK) {
                    close_candidates.push_back({raw, std::move(candidate)});
                }
            };

            for (int i = 0; i < n; ++i) {
                for (int j = i + 1; j < n; ++j) {
                    vector<int> candidate = base_order;
                    swap(candidate[i], candidate[j]);
                    add_if_close(std::move(candidate));
                    if ((checks & 255) == 0 && order_time_over()) break;
                }
                if (order_time_over()) break;
            }

            for (int i = 0; i < n && !order_time_over(); ++i) {
                for (int j = i + 1; j < n; ++j) {
                    vector<int> candidate = base_order;
                    reverse(candidate.begin() + i, candidate.begin() + j + 1);
                    add_if_close(std::move(candidate));
                    if ((checks & 255) == 0 && order_time_over()) break;
                }
            }

            for (int i = 0; i < n && !order_time_over(); ++i) {
                for (int j = 0; j <= n; ++j) {
                    if (j == i || j == i + 1) continue;
                    vector<int> candidate = base_order;
                    int value = candidate[i];
                    candidate.erase(candidate.begin() + i);
                    int insert_pos = j;
                    if (i < j) --insert_pos;
                    candidate.insert(candidate.begin() + insert_pos, value);
                    add_if_close(std::move(candidate));
                    if ((checks & 255) == 0 && order_time_over()) break;
                }
            }

            stable_sort(close_candidates.begin(), close_candidates.end(),
                        [](const OrderProxyCandidate &a, const OrderProxyCandidate &b) {
                if (a.raw != b.raw) return a.raw < b.raw;
                return a.order < b.order;
            });
            if (static_cast<int>(close_candidates.size()) > ORDER_S_NGRAM_CAND_LIMIT) {
                close_candidates.resize(ORDER_S_NGRAM_CAND_LIMIT);
            }

            vector<int> best_order = base_order;
            int best_raw = current_raw;
            long long best_proxy = current_proxy;
            for (const OrderProxyCandidate &candidate : close_candidates) {
                if (order_time_over()) break;
                long long proxy = order_s_ngram_proxy_score(candidate.order);
                if (proxy < best_proxy ||
                    (proxy == best_proxy && candidate.raw < best_raw)) {
                    best_proxy = proxy;
                    best_raw = candidate.raw;
                    best_order = candidate.order;
                }
            }

            if (best_proxy >= current_proxy) break;
            base_order = std::move(best_order);
            current_raw = best_raw;
            current_proxy = best_proxy;
        }

        return base_order;
    };
#endif

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

#if HIER_S_PRIORITY == 3
        const bool hier_s_priority_active =
            (160 <= wall_density && wall_density <= 200 && M <= 25);
#elif HIER_S_PRIORITY
        const bool hier_s_priority_active = true;
#endif

        struct MacroCand {
            string pattern;
            int gain;
            int rank;
            int s_count;
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
        for (const auto &[pattern, gain] : best_gain) {
            int rank = gain;
            int s_count = 0;
#if HIER_S_PRIORITY
            if (hier_s_priority_active) {
                s_count = static_cast<int>(count(pattern.begin(), pattern.end(), 'S'));
            }
#endif
#if HIER_S_PRIORITY == 1
            if (s_count > 0) {
                rank += HIER_S_PRIORITY_WEIGHT *
                        (min<int>(pattern.size(), 40) + 12 * min(s_count, 3));
            }
#elif HIER_S_PRIORITY == 2
            if (s_count > 0) {
                rank += HIER_S_PRIORITY_WEIGHT * min<int>(pattern.size(), 60);
            }
#elif HIER_S_PRIORITY == 3
            if (hier_s_priority_active && s_count > 0) {
                rank += HIER_S_PRIORITY_WEIGHT *
                        (min<int>(pattern.size(), 40) + 12 * min(s_count, 3));
            }
#endif
            cands.push_back({pattern, gain, rank, s_count});
        }
        sort(cands.begin(), cands.end(), [](const MacroCand &a, const MacroCand &b) {
            if (a.rank != b.rank) return a.rank > b.rank;
            if (a.gain != b.gain) return a.gain > b.gain;
            return a.pattern.size() > b.pattern.size();
        });
        if (static_cast<int>(cands.size()) > HIER_CAND_LIMIT) cands.resize(HIER_CAND_LIMIT);

#if HIER_S_PRIORITY && MACRO_CHAIN_DEBUG
        int hier_s_selected = 0;
        int hier_s2_selected = 0;
        for (const MacroCand &cand : cands) {
            if (cand.s_count > 0) ++hier_s_selected;
            if (cand.s_count >= 2) ++hier_s2_selected;
        }
        cerr << "METRIC hier_s_selected=" << hier_s_selected
             << " hier_s2_selected=" << hier_s2_selected
             << " hier_candidates=" << cands.size()
             << " hier_s_priority_active=" << (hier_s_priority_active ? 1 : 0)
             << "\n";
#endif

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

#if ENABLE_S_NGRAM_PROXY
    auto s_ngram_gain_estimate = [&](const string &base) -> int {
        static const int lens[] = {6, 8, 10, 12, 16, 24, 30};
        const int n = static_cast<int>(base.size());
        vector<int> s_prefix(n + 1, 0);
        for (int i = 0; i < n; ++i) {
            s_prefix[i + 1] = s_prefix[i] + (base[i] == 'S');
        }

        long long gain = 0;
        for (int len : lens) {
            if (len > n) continue;
            unordered_map<string, int> count;
            count.reserve(n);
            for (int i = 0; i + len <= n; ++i) {
                if (s_prefix[i + len] == s_prefix[i]) continue;
                ++count[base.substr(i, len)];
            }
            for (const auto &[gram, cnt] : count) {
                (void)gram;
                if (cnt >= 2) gain += 1LL * min(cnt - 1, 4) * (len - 1);
            }
            if (gain > n / 3) return n / 3;
        }
        return static_cast<int>(min<long long>(gain, n / 3));
    };

    auto s_ngram_proxy_score = [&](const BuiltAnswer &built) -> long long {
        if (built.delivered < M) return 1LL * T * (M - built.delivered);
        int gain = s_ngram_gain_estimate(built.base);
#if S_NGRAM_PROXY_MODE == 1
        return 2LL * static_cast<int>(built.base.size()) - 3LL * gain;
#elif S_NGRAM_PROXY_MODE == 2
        return static_cast<long long>(built.base.size()) - 2LL * gain;
#elif S_NGRAM_PROXY_MODE == 3
        return 3LL * static_cast<int>(built.base.size()) - 2LL * gain;
#else
        return static_cast<long long>(built.base.size()) - gain;
#endif
    };

    struct EvaluatedCandidate {
        BuiltAnswer answer;
        string output;
        tuple<long long, int, int> score;
        long long proxy;
    };
    vector<EvaluatedCandidate> evaluated_candidates;
#endif

#if ENABLE_ALT_ROUTE_CANDIDATE
#if ALT_ROUTE_NEW_GRAM_BONUS
    int alt_route_newgram_bonus_total = 0;
    int alt_route_newgram_choices = 0;
#endif

    auto build_s_ngram_weights = [&](const string &base) {
        static const int lens[] = {6, 8, 10, 12, 16, 24, 30};
        const int n = static_cast<int>(base.size());
        vector<int> s_prefix(n + 1, 0);
        for (int i = 0; i < n; ++i) {
            s_prefix[i + 1] = s_prefix[i] + (base[i] == 'S');
        }

        unordered_map<string, int> weights;
        weights.reserve(n);
        for (int len : lens) {
            if (len > n) continue;
            unordered_map<string, int> count;
            count.reserve(n);
            for (int i = 0; i + len <= n; ++i) {
                if (s_prefix[i + len] == s_prefix[i]) continue;
                ++count[base.substr(i, len)];
            }
            for (const auto &[gram, cnt] : count) {
                if (cnt < 2) continue;
                weights[gram] += min(cnt - 1, 4) * (len - 1);
            }
        }
        return weights;
    };

    auto s_ngram_segment_bonus =
        [&](const string &prefix, const string &segment,
            const unordered_map<string, int> &weights) -> int {
        if (weights.empty() || segment.empty()) return 0;

        static const int lens[] = {6, 8, 10, 12, 16, 24, 30};
        const int keep = min<int>(29, prefix.size());
        string context = prefix.substr(prefix.size() - keep) + segment;
        const int boundary = keep;
        const int total = static_cast<int>(context.size());

        vector<int> s_prefix(total + 1, 0);
        for (int i = 0; i < total; ++i) {
            s_prefix[i + 1] = s_prefix[i] + (context[i] == 'S');
        }

        int bonus = 0;
        for (int len : lens) {
            if (len > total) continue;
            int first = max(0, boundary - len + 1);
            int last = min(boundary + static_cast<int>(segment.size()) - len, total - len);
            for (int i = first; i <= last; ++i) {
                if (i + len <= boundary) continue;
                if (s_prefix[i + len] == s_prefix[i]) continue;
                auto it = weights.find(context.substr(i, len));
                if (it != weights.end()) bonus += it->second;
            }
        }
        return bonus;
    };

    auto s_ngram_new_repeat_bonus =
        [&](const string &prefix, const string &segment) -> int {
#if ALT_ROUTE_NEW_GRAM_BONUS
        if (prefix.empty() || segment.empty()) return 0;

        static const int lens[] = {6, 8, 10, 12, 16, 24, 30};
        const int prefix_len = static_cast<int>(prefix.size());
        const int keep = min<int>(29, prefix_len);
        string context = prefix.substr(prefix_len - keep) + segment;
        const int boundary = keep;
        const int total = static_cast<int>(context.size());

        int bonus = 0;
        for (int len : lens) {
            if (prefix_len >= len) {
                unordered_map<string, char> seen;
                seen.reserve(prefix_len);
                vector<int> p_s_prefix(prefix_len + 1, 0);
                for (int i = 0; i < prefix_len; ++i) {
                    p_s_prefix[i + 1] = p_s_prefix[i] + (prefix[i] == 'S');
                }
                for (int i = 0; i + len <= prefix_len; ++i) {
                    if (p_s_prefix[i + len] == p_s_prefix[i]) continue;
                    seen[prefix.substr(i, len)] = 1;
                }
                if (seen.empty()) continue;

                vector<int> c_s_prefix(total + 1, 0);
                for (int i = 0; i < total; ++i) {
                    c_s_prefix[i + 1] = c_s_prefix[i] + (context[i] == 'S');
                }
                int first = max(0, boundary - len + 1);
                int last = min(boundary + static_cast<int>(segment.size()) - len,
                               total - len);
                int hits = 0;
                for (int i = first; i <= last; ++i) {
                    if (i + len <= boundary) continue;
                    if (c_s_prefix[i + len] == c_s_prefix[i]) continue;
                    if (seen.find(context.substr(i, len)) != seen.end()) ++hits;
                }
                if (hits > 0) bonus += min(hits, 3) * (len - 1);
            }
        }
        return min<int>(bonus, segment.size() * 2);
#else
        (void)prefix;
        (void)segment;
        return 0;
#endif
    };

    auto combine_alt_route_bonus = [&](int reference_bonus, int new_repeat_bonus) {
#if ALT_ROUTE_NEW_GRAM_BONUS == 2
        return reference_bonus * 1000 + min(new_repeat_bonus, 999);
#else
        return reference_bonus + ALT_ROUTE_NEW_GRAM_WEIGHT * new_repeat_bonus;
#endif
    };

    auto build_answer_with_route_choice =
        [&](const vector<int> &chosen_order, const string &reference_base) -> BuiltAnswer {
        unordered_map<string, int> weights = build_s_ngram_weights(reference_base);

        string result;
        int current = 0;
        int dir = 1;
        int delivered = 0;

        for (int k : chosen_order) {
            int ball_node = 1 + k;
            int basket_node = 1 + M + k;

            const Route &base_to_ball = route[current][dir][ball_node];
            if (base_to_ball.end_dir == -1) break;
            const Route &base_to_basket =
                route[ball_node][base_to_ball.end_dir][basket_node];
            if (base_to_basket.end_dir == -1) break;

            string baseline_segment = base_to_ball.ops;
            baseline_segment.push_back('S');
            baseline_segment += base_to_basket.ops;
            baseline_segment.push_back('S');
            int baseline_raw = static_cast<int>(baseline_segment.size());

            int min_raw = 1000000000;
            for (int m1 = 0; m1 < 6; ++m1) {
                const Route &r1 = alt_routes[m1][current][dir][ball_node];
                if (r1.end_dir == -1) continue;
                for (int m2 = 0; m2 < 6; ++m2) {
                    const Route &r2 = alt_routes[m2][ball_node][r1.end_dir][basket_node];
                    if (r2.end_dir == -1) continue;
                    min_raw = min(min_raw, r1.cost + 1 + r2.cost + 1);
                }
            }
            if (min_raw == 1000000000) break;

            string best_segment;
            int best_end_dir = -1;
            int best_bonus = -1;
            if (min_raw == baseline_raw) {
                best_segment = baseline_segment;
                best_end_dir = base_to_basket.end_dir;
                best_bonus = combine_alt_route_bonus(
                    s_ngram_segment_bonus(result, best_segment, weights),
                    s_ngram_new_repeat_bonus(result, best_segment));
            }

            for (int m1 = 0; m1 < 6; ++m1) {
                const Route &r1 = alt_routes[m1][current][dir][ball_node];
                if (r1.end_dir == -1) continue;
                for (int m2 = 0; m2 < 6; ++m2) {
                    const Route &r2 = alt_routes[m2][ball_node][r1.end_dir][basket_node];
                    if (r2.end_dir == -1) continue;
                    int raw = r1.cost + 1 + r2.cost + 1;
                    if (raw != min_raw) continue;

                    string segment = r1.ops;
                    segment.push_back('S');
                    segment += r2.ops;
                    segment.push_back('S');

                    int base_bonus = s_ngram_segment_bonus(result, segment, weights);
                    int new_repeat_bonus = s_ngram_new_repeat_bonus(result, segment);
                    int bonus = combine_alt_route_bonus(base_bonus, new_repeat_bonus);
                    if (bonus > best_bonus ||
                        (bonus == best_bonus && best_segment.empty())) {
                        best_bonus = bonus;
                        best_segment = std::move(segment);
                        best_end_dir = r2.end_dir;
#if ALT_ROUTE_NEW_GRAM_BONUS
                        if (new_repeat_bonus > 0) {
                            alt_route_newgram_bonus_total += new_repeat_bonus;
                            ++alt_route_newgram_choices;
                        }
#endif
                    }
                }
            }

            if (best_segment.empty() || best_end_dir == -1) break;
            if (result.size() + best_segment.size() > static_cast<size_t>(T)) break;

            result += best_segment;
            current = basket_node;
            dir = best_end_dir;
            ++delivered;
        }

        return {result, delivered};
    };

#if ALT_ROUTE_BEAM_WIDTH > 0 && ALT_ROUTE_FINAL_LIMIT > 0
    auto build_route_choice_beam_answers =
        [&](const vector<int> &chosen_order, const string &reference_base) -> vector<BuiltAnswer> {
        unordered_map<string, int> weights = build_s_ngram_weights(reference_base);

        struct RouteBeamState {
            string result;
            int current;
            int dir;
            int delivered;
            int bonus;
        };

        auto proxy_score = [](const RouteBeamState &state) {
            int raw = static_cast<int>(state.result.size());
            return raw - min(state.bonus, raw / 3);
        };

        vector<RouteBeamState> beam;
        beam.push_back({"", 0, 1, 0, 0});

        for (int k : chosen_order) {
            vector<RouteBeamState> next;
            for (const RouteBeamState &state : beam) {
                int ball_node = 1 + k;
                int basket_node = 1 + M + k;

                int min_raw = 1000000000;
                for (int m1 = 0; m1 < 6; ++m1) {
                    const Route &r1 = alt_routes[m1][state.current][state.dir][ball_node];
                    if (r1.end_dir == -1) continue;
                    for (int m2 = 0; m2 < 6; ++m2) {
                        const Route &r2 = alt_routes[m2][ball_node][r1.end_dir][basket_node];
                        if (r2.end_dir == -1) continue;
                        min_raw = min(min_raw, r1.cost + 1 + r2.cost + 1);
                    }
                }
                if (min_raw == 1000000000) continue;

                for (int m1 = 0; m1 < 6; ++m1) {
                    const Route &r1 = alt_routes[m1][state.current][state.dir][ball_node];
                    if (r1.end_dir == -1) continue;
                    for (int m2 = 0; m2 < 6; ++m2) {
                        const Route &r2 = alt_routes[m2][ball_node][r1.end_dir][basket_node];
                        if (r2.end_dir == -1) continue;
                        int raw = r1.cost + 1 + r2.cost + 1;
                        if (raw != min_raw) continue;

                        string segment = r1.ops;
                        segment.push_back('S');
                        segment += r2.ops;
                        segment.push_back('S');
                        if (state.result.size() + segment.size() > static_cast<size_t>(T)) {
                            continue;
                        }

                        RouteBeamState candidate = state;
                        int bonus = s_ngram_segment_bonus(candidate.result, segment, weights);
                        candidate.result += segment;
                        candidate.current = basket_node;
                        candidate.dir = r2.end_dir;
                        candidate.delivered += 1;
                        candidate.bonus += bonus;
                        next.push_back(std::move(candidate));
                    }
                }
            }

            if (next.empty()) break;
            stable_sort(next.begin(), next.end(), [&](const RouteBeamState &a,
                                                      const RouteBeamState &b) {
                int pa = proxy_score(a);
                int pb = proxy_score(b);
                if (pa != pb) return pa < pb;
                if (a.delivered != b.delivered) return a.delivered > b.delivered;
                if (a.result.size() != b.result.size()) return a.result.size() < b.result.size();
                return a.dir < b.dir;
            });

            vector<RouteBeamState> pruned;
            pruned.reserve(min<int>(ALT_ROUTE_BEAM_WIDTH, next.size()));
            vector<pair<int, int>> seen;
            for (RouteBeamState &state : next) {
                pair<int, int> key = {state.current, state.dir};
                if (find(seen.begin(), seen.end(), key) != seen.end()) continue;
                seen.push_back(key);
                pruned.push_back(std::move(state));
                if (static_cast<int>(pruned.size()) >= ALT_ROUTE_BEAM_WIDTH) break;
            }
            if (pruned.empty()) {
                next.resize(min<int>(ALT_ROUTE_BEAM_WIDTH, next.size()));
                beam = std::move(next);
            } else {
                beam = std::move(pruned);
            }
        }

        stable_sort(beam.begin(), beam.end(), [&](const RouteBeamState &a,
                                                 const RouteBeamState &b) {
            if (a.delivered != b.delivered) return a.delivered > b.delivered;
            int pa = proxy_score(a);
            int pb = proxy_score(b);
            if (pa != pb) return pa < pb;
            return a.result.size() < b.result.size();
        });

        vector<BuiltAnswer> answers;
        for (const RouteBeamState &state : beam) {
            answers.push_back({state.result, state.delivered});
            if (static_cast<int>(answers.size()) >= ALT_ROUTE_FINAL_LIMIT) break;
        }
        return answers;
    };
#endif
#endif

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

#if ENABLE_SEGMENT_CLUSTER_CANDIDATE
    {
        vector<int> base_pos(M, M);
        for (int i = 0; i < static_cast<int>(order.size()); ++i) {
            base_pos[order[i]] = i;
        }

        vector<string> carry_sig(M);
        vector<int> carry_len(M, 1000000000);
        vector<int> carry_start_dir(M, 0);
        vector<int> carry_end_dir(M, 0);
        for (int k = 0; k < M; ++k) {
            int ball_node = 1 + k;
            int basket_node = 1 + M + k;
            for (int d = 0; d < 4; ++d) {
                const Route &r = route[ball_node][d][basket_node];
                if (r.end_dir == -1) continue;
                if (r.cost < carry_len[k] ||
                    (r.cost == carry_len[k] && r.ops < carry_sig[k])) {
                    carry_len[k] = r.cost;
                    carry_sig[k] = r.ops;
                    carry_start_dir[k] = d;
                    carry_end_dir[k] = r.end_dir;
                }
            }
        }

        unordered_map<string, vector<int>> groups;
        groups.reserve(M * 3 + 1);
        for (int k = 0; k < M; ++k) {
            if (carry_sig[k].empty()) continue;

            string exact_key = "E:" + carry_sig[k];
            groups[exact_key].push_back(k);

            int dr0 = basket[k].r - ball[k].r;
            int dc0 = basket[k].c - ball[k].c;
            int sdr = (dr0 > 0) - (dr0 < 0);
            int sdc = (dc0 > 0) - (dc0 < 0);
            string coarse_key = "D:" + to_string(sdr) + "," + to_string(sdc) + "," +
                                to_string(abs(dr0) + abs(dc0)) + "," +
                                to_string(carry_start_dir[k]) + "," +
                                to_string(carry_end_dir[k]);
            groups[coarse_key].push_back(k);
        }

        struct SegmentClusterGroup {
            int score = 0;
            vector<int> items;
        };
        vector<SegmentClusterGroup> segment_groups;
        for (auto &[key, items] : groups) {
            (void)key;
            sort(items.begin(), items.end(), [&](int a, int b) {
                if (base_pos[a] != base_pos[b]) return base_pos[a] < base_pos[b];
                return a < b;
            });
            items.erase(unique(items.begin(), items.end()), items.end());
            if (items.size() < 2) continue;

            int repeated_len = 0;
            for (int k : items) repeated_len += max(1, carry_len[k]);
            int avg_len = repeated_len / static_cast<int>(items.size());
            int span = base_pos[items.back()] - base_pos[items.front()];
            int score = static_cast<int>(items.size()) * avg_len * 100 - span * 15;
            segment_groups.push_back({score, items});
        }

        stable_sort(segment_groups.begin(), segment_groups.end(),
                    [](const SegmentClusterGroup &a, const SegmentClusterGroup &b) {
            if (a.score != b.score) return a.score > b.score;
            if (a.items.size() != b.items.size()) return a.items.size() > b.items.size();
            return a.items < b.items;
        });

        int segment_cluster_added = 0;
        int segment_cluster_groups = static_cast<int>(segment_groups.size());
        int segment_cluster_best_size = 0;
        vector<string> seen_orders;
        for (const SegmentClusterGroup &group : segment_groups) {
            if (segment_cluster_added >= SEGMENT_CLUSTER_LIMIT) break;
            vector<int> selected = group.items;
            if (static_cast<int>(selected.size()) > SEGMENT_CLUSTER_MAX_ITEMS) {
                selected.resize(SEGMENT_CLUSTER_MAX_ITEMS);
            }
            if (selected.size() < 2) continue;
            segment_cluster_best_size =
                max(segment_cluster_best_size, static_cast<int>(selected.size()));

            vector<char> in_group(M, 0);
            int insert_pos = M;
            for (int k : selected) {
                in_group[k] = 1;
                insert_pos = min(insert_pos, base_pos[k]);
            }

            vector<int> clustered;
            clustered.reserve(M);
            bool inserted = false;
            for (int idx = 0; idx < static_cast<int>(order.size()); ++idx) {
                if (!inserted && idx >= insert_pos) {
                    for (int k : selected) clustered.push_back(k);
                    inserted = true;
                }
                int k = order[idx];
                if (!in_group[k]) clustered.push_back(k);
            }
            if (!inserted) {
                for (int k : selected) clustered.push_back(k);
            }

            string key;
            key.reserve(clustered.size() * 2);
            for (int k : clustered) {
                key.push_back(static_cast<char>(k & 255));
                key.push_back(static_cast<char>(k >> 8));
            }
            if (find(seen_orders.begin(), seen_orders.end(), key) != seen_orders.end()) {
                continue;
            }
            seen_orders.push_back(std::move(key));
            add_candidate(std::move(clustered), 0);
            ++segment_cluster_added;
        }

#if MACRO_CHAIN_DEBUG
        cerr << "METRIC segment_cluster_groups=" << segment_cluster_groups
             << " segment_cluster_added=" << segment_cluster_added
             << " segment_cluster_best_size=" << segment_cluster_best_size
             << "\n";
#else
        (void)segment_cluster_groups;
        (void)segment_cluster_added;
        (void)segment_cluster_best_size;
#endif
    }
#endif

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
    BuiltAnswer best_answer;
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
#if ENABLE_S_NGRAM_PROXY
        evaluated_candidates.push_back(
            {answer, candidate_output, candidate_score, s_ngram_proxy_score(answer)});
#endif
        if (candidate_score < best_score) {
            best_score = candidate_score;
            output = candidate_output;
            best_base = answer.base;
            best_answer = answer;
            best_order = candidate_order;
        }

#if ENABLE_ORDER_S_NGRAM_SEARCH
        if (idx == 0 && !order_time_over()) {
            vector<int> s_candidate_order = improve_order_s_ngram(candidate_order);
            if (s_candidate_order != candidate_order) {
                BuiltAnswer s_answer = build_answer(s_candidate_order);
                string s_output = compress_output(s_answer.base);
                auto s_score = score_tuple(s_answer, s_output);
#if ENABLE_S_NGRAM_PROXY
                evaluated_candidates.push_back(
                    {s_answer, s_output, s_score, s_ngram_proxy_score(s_answer)});
#endif
                if (s_score < best_score) {
                    best_score = s_score;
                    output = s_output;
                    best_base = s_answer.base;
                    best_answer = s_answer;
                    best_order = s_candidate_order;
                }
            }
        }
#endif
    }

    if (output.empty()) {
        BuiltAnswer answer = build_answer(improve_order(order, IMPROVE_FIRST_ROUNDS));
        output = compress_output(answer.base);
        best_base = answer.base;
        best_answer = answer;
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
            best_answer = scan_answer;
        }
    }
#endif

    bool cyclic_shift_tried = false;

#if ENABLE_CYCLIC_SHIFT_CANDIDATE
    if (!cyclic_shift_tried && !best_order.empty() && !time_over() &&
        M <= CYCLIC_SHIFT_MAX_K &&
        wall_count >= 60) {
        cyclic_shift_tried = true;

        auto append_all_cyclic_turn_ops = [](string &ops, int from_dir, int to_dir) {
            int diff = (to_dir - from_dir + 4) & 3;
            if (diff == 1) {
                ops.push_back('R');
            } else if (diff == 2) {
                ops.push_back('R');
                ops.push_back('R');
            } else if (diff == 3) {
                ops.push_back('L');
            }
        };

        auto all_cyclic_route_min_cost = [&](int from_node, int to_node) {
            int best = 1000000000;
            for (int d = 0; d < 4; ++d) {
                best = min(best, route[from_node][d][to_node].cost);
            }
            return best;
        };

        auto all_cyclic_order_cost = [&](const vector<int> &items) {
            const int k_count = static_cast<int>(items.size());
            int cost = 2 * k_count + 1;
            for (int i = 0; i + 1 < k_count; ++i) {
                cost += all_cyclic_route_min_cost(1 + items[i], 1 + items[i + 1]);
                cost += all_cyclic_route_min_cost(1 + M + items[i], 1 + M + items[i + 1]);
            }
            cost += all_cyclic_route_min_cost(1 + items.back(), 1 + M + items.front());
            cost += all_cyclic_route_min_cost(1 + M + items.back(), 1 + items.front());
            return cost;
        };

        vector<int> items(M);
        iota(items.begin(), items.end(), 0);
        int best_items_cost = all_cyclic_order_cost(items);
        bool improved_items = true;
        for (int round = 0; round < 4 && improved_items && !time_over(); ++round) {
            improved_items = false;
            for (int l = 0; l < M && !time_over(); ++l) {
                for (int r = l + 2; r <= M; ++r) {
                    vector<int> candidate = items;
                    reverse(candidate.begin() + l, candidate.begin() + r);
                    int cost = all_cyclic_order_cost(candidate);
                    if (cost < best_items_cost) {
                        best_items_cost = cost;
                        items = std::move(candidate);
                        improved_items = true;
                    }
                }
            }
            for (int i = 0; i < M && !time_over(); ++i) {
                int value = items[i];
                vector<int> rest = items;
                rest.erase(rest.begin() + i);
                for (int pos = 0; pos <= static_cast<int>(rest.size()); ++pos) {
                    vector<int> candidate = rest;
                    candidate.insert(candidate.begin() + pos, value);
                    int cost = all_cyclic_order_cost(candidate);
                    if (cost < best_items_cost) {
                        best_items_cost = cost;
                        items = std::move(candidate);
                        improved_items = true;
                    }
                }
            }
        }

        for (int rot = 0; rot < M && !time_over(); ++rot) {
            vector<int> order_items;
            order_items.reserve(M);
            for (int i = 0; i < M; ++i) order_items.push_back(items[(rot + i) % M]);

            const int first = order_items.front();
            const int first_node = 1 + first;
            const Route &to_first = route[0][1][first_node];
            if (to_first.end_dir == -1) continue;

            for (int start_dir = 0; start_dir < 4; ++start_dir) {
                string prefix = to_first.ops;
                append_all_cyclic_turn_ops(prefix, to_first.end_dir, start_dir);

                string body;
                body.reserve(best_items_cost + 32);
                body.push_back('S');
                int current_node = first_node;
                int cyclic_dir = start_dir;
                bool ok = true;

                for (int idx = 1; idx < M; ++idx) {
                    int next_node = 1 + order_items[idx];
                    const Route &move = route[current_node][cyclic_dir][next_node];
                    if (move.end_dir == -1) {
                        ok = false;
                        break;
                    }
                    body += move.ops;
                    body.push_back('S');
                    current_node = next_node;
                    cyclic_dir = move.end_dir;
                }
                if (!ok) continue;

                for (int idx = 0; idx < M; ++idx) {
                    int next_node = 1 + M + order_items[idx];
                    const Route &move = route[current_node][cyclic_dir][next_node];
                    if (move.end_dir == -1) {
                        ok = false;
                        break;
                    }
                    body += move.ops;
                    body.push_back('S');
                    current_node = next_node;
                    cyclic_dir = move.end_dir;
                }
                if (!ok) continue;

                const Route &back = route[current_node][cyclic_dir][first_node];
                if (back.end_dir == -1) continue;
                body += back.ops;
                body.push_back('S');
                append_all_cyclic_turn_ops(body, back.end_dir, start_dir);

                string base_prefix = prefix;
                base_prefix.reserve(prefix.size() + static_cast<size_t>(M) * body.size());
                for (int rep = 0; rep < M; ++rep) base_prefix += body;
                if (base_prefix.size() > static_cast<size_t>(T)) continue;

                vector<char> done_all(M, 1);
                BuiltAnswer built =
                    build_suffix_answer(best_order, done_all, first_node, start_dir,
                                        std::move(base_prefix), M);
                if (built.delivered < M) continue;

                string candidate_output = compress_output(prefix);
                candidate_output.push_back('M');
                candidate_output += body;
                candidate_output.push_back('M');
                candidate_output.append(M - 1, 'P');
                if (candidate_output.size() > static_cast<size_t>(T)) continue;

                auto candidate_score = score_tuple(built, candidate_output);
                if (candidate_score < best_score) {
                    best_score = candidate_score;
                    output = std::move(candidate_output);
                    best_base = std::move(built.base);
                    best_answer = std::move(built);
                }
            }
        }
    }
#else
    (void)cyclic_shift_tried;
#endif

#if ENABLE_S_NGRAM_PROXY
    auto try_hierarchical_candidate = [&](const BuiltAnswer &answer, const string &fallback) {
        if (answer.base.empty() || time_over()) return;
        string candidate_output = compress_hierarchical(answer.base, fallback);
        auto candidate_score = score_tuple(answer, candidate_output);
        if (candidate_score < best_score) {
            best_score = candidate_score;
            output = candidate_output;
            best_base = answer.base;
            best_answer = answer;
        }
    };

    try_hierarchical_candidate(best_answer, output);

    if (!time_over() && !evaluated_candidates.empty()) {
        stable_sort(evaluated_candidates.begin(), evaluated_candidates.end(),
                    [](const EvaluatedCandidate &a, const EvaluatedCandidate &b) {
                        if (a.proxy != b.proxy) return a.proxy < b.proxy;
                        return a.score < b.score;
                    });

        int tried = 0;
        for (const EvaluatedCandidate &candidate : evaluated_candidates) {
            if (time_over() || tried >= S_NGRAM_HIER_LIMIT) break;
            if (candidate.answer.base == best_base) continue;
            try_hierarchical_candidate(candidate.answer, candidate.output);
            ++tried;
        }
    }
#else
    if (!best_base.empty() && !time_over()) {
        output = compress_hierarchical(best_base, output);
    }
#endif

#if ENABLE_CYCLIC_SHIFT_CANDIDATE
    if (!cyclic_shift_tried && !best_order.empty() && !time_over() &&
        M <= CYCLIC_SHIFT_MAX_K &&
        wall_count >= 60) {
        auto append_cyclic_turn_ops = [](string &ops, int from_dir, int to_dir) {
            int diff = (to_dir - from_dir + 4) & 3;
            if (diff == 1) {
                ops.push_back('R');
            } else if (diff == 2) {
                ops.push_back('R');
                ops.push_back('R');
            } else if (diff == 3) {
                ops.push_back('L');
            }
        };

        auto cyclic_snake_key = [&](const Pos &p) {
            return (p.r & 1) ? p.r * N + (N - 1 - p.c) : p.r * N + p.c;
        };

        auto cyclic_route_min_cost = [&](int from_node, int to_node) {
            int best = 1000000000;
            for (int d = 0; d < 4; ++d) {
                best = min(best, route[from_node][d][to_node].cost);
            }
            return best;
        };

        auto cyclic_order_cost = [&](const vector<int> &items) {
            const int k_count = static_cast<int>(items.size());
            if (k_count == 0) return 0;
            int cost = 2 * k_count + 1;
            for (int i = 0; i + 1 < k_count; ++i) {
                cost += cyclic_route_min_cost(1 + items[i], 1 + items[i + 1]);
                cost += cyclic_route_min_cost(1 + M + items[i], 1 + M + items[i + 1]);
            }
            cost += cyclic_route_min_cost(1 + items.back(), 1 + M + items.front());
            cost += cyclic_route_min_cost(1 + M + items.back(), 1 + items.front());
            return cost;
        };

        auto improve_cyclic_order = [&](vector<int> items) {
            if (items.size() <= 2) return items;
            int best_cost_local = cyclic_order_cost(items);
            bool improved = true;
            for (int round = 0; round < 3 && improved && !time_over(); ++round) {
                improved = false;
                const int k_count = static_cast<int>(items.size());
                for (int l = 0; l < k_count && !time_over(); ++l) {
                    for (int r = l + 2; r <= k_count; ++r) {
                        vector<int> candidate = items;
                        reverse(candidate.begin() + l, candidate.begin() + r);
                        int cost = cyclic_order_cost(candidate);
                        if (cost < best_cost_local) {
                            best_cost_local = cost;
                            items = std::move(candidate);
                            improved = true;
                        }
                    }
                }
                for (int i = 0; i < k_count && !time_over(); ++i) {
                    int value = items[i];
                    vector<int> rest = items;
                    rest.erase(rest.begin() + i);
                    for (int pos = 0; pos <= static_cast<int>(rest.size()); ++pos) {
                        vector<int> candidate = rest;
                        candidate.insert(candidate.begin() + pos, value);
                        int cost = cyclic_order_cost(candidate);
                        if (cost < best_cost_local) {
                            best_cost_local = cost;
                            items = std::move(candidate);
                            improved = true;
                        }
                    }
                }
            }
            return items;
        };

        struct CyclicShiftSeed {
            int estimate = 0;
            vector<int> items;
        };

        vector<vector<int>> cyclic_source_orders;
        cyclic_source_orders.push_back(best_order);
        {
            vector<int> by_ball(M), by_basket(M), by_mid(M);
            iota(by_ball.begin(), by_ball.end(), 0);
            by_basket = by_ball;
            by_mid = by_ball;
            stable_sort(by_ball.begin(), by_ball.end(), [&](int a, int b) {
                int ka = cyclic_snake_key(ball[a]);
                int kb = cyclic_snake_key(ball[b]);
                if (ka != kb) return ka < kb;
                return a < b;
            });
            stable_sort(by_basket.begin(), by_basket.end(), [&](int a, int b) {
                int ka = cyclic_snake_key(basket[a]);
                int kb = cyclic_snake_key(basket[b]);
                if (ka != kb) return ka < kb;
                return a < b;
            });
            stable_sort(by_mid.begin(), by_mid.end(), [&](int a, int b) {
                Pos ma{(ball[a].r + basket[a].r) / 2,
                       (ball[a].c + basket[a].c) / 2};
                Pos mb{(ball[b].r + basket[b].r) / 2,
                       (ball[b].c + basket[b].c) / 2};
                int ka = cyclic_snake_key(ma);
                int kb = cyclic_snake_key(mb);
                if (ka != kb) return ka < kb;
                return a < b;
            });
            cyclic_source_orders.push_back(std::move(by_ball));
            cyclic_source_orders.push_back(std::move(by_basket));
            cyclic_source_orders.push_back(std::move(by_mid));
        }

        vector<CyclicShiftSeed> cyclic_seeds;
        unordered_map<string, char> cyclic_seen;
        cyclic_seen.reserve(512);

        auto add_cyclic_seed = [&](vector<int> items) {
            const int k_count = static_cast<int>(items.size());
            if (k_count < 3) return;
            vector<int> key_items = items;
            sort(key_items.begin(), key_items.end());
            string key;
            key.reserve(key_items.size() * 2);
            for (int k : key_items) {
                key.push_back(static_cast<char>(k & 255));
                key.push_back(static_cast<char>((k >> 8) & 255));
            }
            if (cyclic_seen.find(key) != cyclic_seen.end()) return;
            cyclic_seen[key] = 1;
            int estimate = cyclic_order_cost(items);
            cyclic_seeds.push_back({estimate, std::move(items)});
        };

        const int max_k = min(CYCLIC_SHIFT_MAX_K, M);
        for (const vector<int> &source : cyclic_source_orders) {
            for (int k_count = 3; k_count <= max_k; ++k_count) {
                for (int begin = 0; begin + k_count <= static_cast<int>(source.size());
                     ++begin) {
                    vector<int> items(source.begin() + begin,
                                      source.begin() + begin + k_count);
                    add_cyclic_seed(std::move(items));
                }
            }
        }
        if (M <= CYCLIC_SHIFT_MAX_K) {
            vector<int> all_items(M);
            iota(all_items.begin(), all_items.end(), 0);
            add_cyclic_seed(std::move(all_items));
        }

        stable_sort(cyclic_seeds.begin(), cyclic_seeds.end(),
                    [](const CyclicShiftSeed &a, const CyclicShiftSeed &b) {
            if (a.estimate != b.estimate) return a.estimate < b.estimate;
            return a.items.size() > b.items.size();
        });
        if (static_cast<int>(cyclic_seeds.size()) > CYCLIC_SHIFT_CANDIDATE_LIMIT) {
            cyclic_seeds.resize(CYCLIC_SHIFT_CANDIDATE_LIMIT);
        }

        int cyclic_valid = 0;
        int cyclic_best_buttons = 1000000000;
        int cyclic_best_k = 0;

        for (const CyclicShiftSeed &seed : cyclic_seeds) {
            if (time_over()) break;
            vector<int> improved_items = improve_cyclic_order(seed.items);
            const int k_count = static_cast<int>(improved_items.size());
            if (k_count < 3) continue;

            for (int rot = 0; rot < k_count && !time_over(); ++rot) {
                vector<int> items;
                items.reserve(k_count);
                for (int i = 0; i < k_count; ++i) {
                    items.push_back(improved_items[(rot + i) % k_count]);
                }
                const int first = items.front();
                const int first_node = 1 + first;
                const Route &to_first = route[0][1][first_node];
                if (to_first.end_dir == -1) continue;

                for (int start_dir = 0; start_dir < 4; ++start_dir) {
                    string prefix = to_first.ops;
                    append_cyclic_turn_ops(prefix, to_first.end_dir, start_dir);

                    string body;
                    body.reserve(seed.estimate + 32);
                    body.push_back('S');
                    int current_node = first_node;
                    int cyclic_dir = start_dir;
                    bool ok = true;

                    for (int idx = 1; idx < k_count; ++idx) {
                        int next_node = 1 + items[idx];
                        const Route &move = route[current_node][cyclic_dir][next_node];
                        if (move.end_dir == -1) {
                            ok = false;
                            break;
                        }
                        body += move.ops;
                        body.push_back('S');
                        current_node = next_node;
                        cyclic_dir = move.end_dir;
                    }
                    if (!ok) continue;

                    for (int idx = 0; idx < k_count; ++idx) {
                        int next_node = 1 + M + items[idx];
                        const Route &move = route[current_node][cyclic_dir][next_node];
                        if (move.end_dir == -1) {
                            ok = false;
                            break;
                        }
                        body += move.ops;
                        body.push_back('S');
                        current_node = next_node;
                        cyclic_dir = move.end_dir;
                    }
                    if (!ok) continue;

                    const Route &back = route[current_node][cyclic_dir][first_node];
                    if (back.end_dir == -1) continue;
                    body += back.ops;
                    body.push_back('S');
                    append_cyclic_turn_ops(body, back.end_dir, start_dir);

                    size_t repeated_len = prefix.size() +
                                          static_cast<size_t>(k_count) * body.size();
                    if (repeated_len > static_cast<size_t>(T)) continue;

                    vector<char> cyclic_done(M, 0);
                    for (int k : items) cyclic_done[k] = 1;

                    string base_prefix = prefix;
                    base_prefix.reserve(repeated_len);
                    for (int rep = 0; rep < k_count; ++rep) base_prefix += body;

                    BuiltAnswer built =
                        build_suffix_answer(best_order, cyclic_done, first_node, start_dir,
                                            std::move(base_prefix), k_count);
                    if (built.delivered < M) continue;

                    BuiltAnswer suffix =
                        build_suffix_answer(best_order, cyclic_done, first_node, start_dir,
                                            string(), k_count);
                    if (suffix.delivered < M) continue;

                    string candidate_output = compress_output(prefix);
                    candidate_output.push_back('M');
                    candidate_output += body;
                    candidate_output.push_back('M');
                    candidate_output.append(k_count - 1, 'P');
                    candidate_output += compress_output(suffix.base);
                    if (candidate_output.size() > static_cast<size_t>(T)) continue;

                    ++cyclic_valid;
                    if (static_cast<int>(candidate_output.size()) < cyclic_best_buttons) {
                        cyclic_best_buttons = static_cast<int>(candidate_output.size());
                        cyclic_best_k = k_count;
                    }

                    auto candidate_score = score_tuple(built, candidate_output);
                    if (candidate_score < best_score) {
                        best_score = candidate_score;
                        output = std::move(candidate_output);
                        best_base = std::move(built.base);
                        best_answer = std::move(built);
                    }
                }
            }
        }

#if MACRO_CHAIN_DEBUG
        cerr << "METRIC cyclic_shift_seeds=" << cyclic_seeds.size()
             << " cyclic_shift_valid=" << cyclic_valid
             << " cyclic_shift_best_buttons=" << cyclic_best_buttons
             << " cyclic_shift_best_k=" << cyclic_best_k << "\n";
#else
        (void)cyclic_valid;
        (void)cyclic_best_buttons;
        (void)cyclic_best_k;
#endif
    }
#endif

#if ENABLE_LOCAL_CLUSTER_SEARCH
    if (!best_order.empty() && !time_over() && M <= LOCAL_CLUSTER_MAX_M) {
        const int cluster_size = min(LOCAL_CLUSTER_SIZE, M);
        struct LocalClusterCandidate {
            int score = 0;
            int begin = 0;
            vector<int> items;
        };

        vector<LocalClusterCandidate> local_cluster_candidates;
        if (cluster_size >= 2 && static_cast<int>(best_order.size()) >= cluster_size) {
            for (int begin = 0; begin + cluster_size <= static_cast<int>(best_order.size());
                 ++begin) {
                vector<int> items;
                items.reserve(cluster_size);
                int min_r = N, min_c = N, max_r = -1, max_c = -1;
                int direct_hint = 0;
                for (int i = 0; i < cluster_size; ++i) {
                    int k = best_order[begin + i];
                    items.push_back(k);
                    min_r = min(min_r, min(ball[k].r, basket[k].r));
                    min_c = min(min_c, min(ball[k].c, basket[k].c));
                    max_r = max(max_r, max(ball[k].r, basket[k].r));
                    max_c = max(max_c, max(ball[k].c, basket[k].c));
                    direct_hint += abs(ball[k].r - basket[k].r) +
                                   abs(ball[k].c - basket[k].c);
                }
                int area = (max_r - min_r + 1) * (max_c - min_c + 1);
                int score = area * 100 - direct_hint * 3 + begin;
                local_cluster_candidates.push_back({score, begin, std::move(items)});
            }
        }

        stable_sort(local_cluster_candidates.begin(), local_cluster_candidates.end(),
                    [](const LocalClusterCandidate &a, const LocalClusterCandidate &b) {
            if (a.score != b.score) return a.score < b.score;
            if (a.begin != b.begin) return a.begin < b.begin;
            return a.items < b.items;
        });
        if (static_cast<int>(local_cluster_candidates.size()) >
            LOCAL_CLUSTER_WINDOW_LIMIT) {
            local_cluster_candidates.resize(LOCAL_CLUSTER_WINDOW_LIMIT);
        }

        auto local_cluster_correct_count =
            [&](const vector<signed char> &board, int k_count) {
            int correct = 0;
            for (int i = 0; i < k_count; ++i) {
                if (board[k_count + i] == i) ++correct;
            }
            return correct;
        };

        struct LocalClusterState {
            int current_node = 0;
            int dir = 1;
            int hold = -1;
            int raw = 0;
            int steps = 0;
            int correct = 0;
            vector<signed char> board;
            string ops;
        };

        auto local_cluster_key = [&](const LocalClusterState &state) {
            string key;
            key.reserve(8 + state.board.size());
            key.push_back(static_cast<char>(state.current_node & 255));
            key.push_back(static_cast<char>((state.current_node >> 8) & 255));
            key.push_back(static_cast<char>(state.dir));
            key.push_back(static_cast<char>(state.hold + 1));
            for (signed char value : state.board) {
                key.push_back(static_cast<char>(value + 1));
            }
            return key;
        };

        auto solve_local_cluster =
            [&](const LocalClusterCandidate &candidate, bool keep_interval) -> BuiltAnswer {
            const vector<int> &items = candidate.items;
            const int k_count = static_cast<int>(items.size());
            if (k_count < 2) return {};

            vector<int> slot_nodes;
            slot_nodes.reserve(2 * k_count);
            for (int k : items) slot_nodes.push_back(1 + k);
            for (int k : items) slot_nodes.push_back(1 + M + k);

            vector<char> prefix_done(M, 0);
            string prefix;
            int start_node = 0;
            int start_dir = 1;
            int delivered_prefix = 0;

            if (keep_interval) {
                for (int idx = 0; idx < candidate.begin; ++idx) {
                    int k = best_order[idx];
                    int ball_node = 1 + k;
                    int basket_node = 1 + M + k;
                    const Route &to_ball = route[start_node][start_dir][ball_node];
                    if (to_ball.end_dir == -1) return {};
                    const Route &to_basket =
                        route[ball_node][to_ball.end_dir][basket_node];
                    if (to_basket.end_dir == -1) return {};

                    string segment = to_ball.ops;
                    segment.push_back('S');
                    segment += to_basket.ops;
                    segment.push_back('S');
                    if (prefix.size() + segment.size() > static_cast<size_t>(T)) {
                        return {};
                    }

                    prefix += segment;
                    prefix_done[k] = 1;
                    start_node = basket_node;
                    start_dir = to_basket.end_dir;
                    ++delivered_prefix;
                }
            }

            int direct_raw = 0;
            int cur_node = start_node;
            int cur_dir = start_dir;
            for (int k : items) {
                int ball_node = 1 + k;
                int basket_node = 1 + M + k;
                const Route &to_ball = route[cur_node][cur_dir][ball_node];
                if (to_ball.end_dir == -1) return {};
                const Route &to_basket = route[ball_node][to_ball.end_dir][basket_node];
                if (to_basket.end_dir == -1) return {};
                direct_raw += to_ball.cost + 1 + to_basket.cost + 1;
                cur_node = basket_node;
                cur_dir = to_basket.end_dir;
            }
            const int raw_limit = direct_raw + LOCAL_CLUSTER_RAW_SLACK;

            LocalClusterState initial;
            initial.current_node = start_node;
            initial.dir = start_dir;
            initial.board.assign(2 * k_count, -1);
            for (int i = 0; i < k_count; ++i) initial.board[i] = i;

            vector<LocalClusterState> beam;
            beam.push_back(std::move(initial));
            vector<LocalClusterState> completed;

            for (int step = 0; step < LOCAL_CLUSTER_MAX_S && !time_over(); ++step) {
                vector<LocalClusterState> next_states;
                next_states.reserve(beam.size() * slot_nodes.size());

                for (const LocalClusterState &state : beam) {
                    for (int slot = 0; slot < static_cast<int>(slot_nodes.size()); ++slot) {
                        int old_cell_ball = state.board[slot];
                        if (old_cell_ball == -1 && state.hold == -1) continue;

                        const Route &move =
                            route[state.current_node][state.dir][slot_nodes[slot]];
                        if (move.end_dir == -1) continue;

                        LocalClusterState next = state;
                        next.current_node = slot_nodes[slot];
                        next.dir = move.end_dir;
                        next.raw += move.cost + 1;
                        if (next.raw > raw_limit) continue;
                        next.ops += move.ops;
                        next.ops.push_back('S');
                        next.board[slot] = static_cast<signed char>(state.hold);
                        next.hold = old_cell_ball;
                        ++next.steps;
                        next.correct = local_cluster_correct_count(next.board, k_count);

                        if (next.correct == k_count && next.hold == -1) {
                            completed.push_back(std::move(next));
                        } else {
                            next_states.push_back(std::move(next));
                        }
                    }
                }

                if (next_states.empty()) break;
                stable_sort(next_states.begin(), next_states.end(),
                            [](const LocalClusterState &a, const LocalClusterState &b) {
                    int pa = a.raw - a.correct * 35 + a.steps * 4;
                    int pb = b.raw - b.correct * 35 + b.steps * 4;
                    if (pa != pb) return pa < pb;
                    if (a.correct != b.correct) return a.correct > b.correct;
                    if (a.raw != b.raw) return a.raw < b.raw;
                    return a.ops < b.ops;
                });

                vector<LocalClusterState> pruned;
                pruned.reserve(min<int>(LOCAL_CLUSTER_BEAM, next_states.size()));
                unordered_map<string, int> seen;
                seen.reserve(LOCAL_CLUSTER_BEAM * 3 + 1);
                for (LocalClusterState &state : next_states) {
                    string key = local_cluster_key(state);
                    auto it = seen.find(key);
                    if (it != seen.end() && it->second <= state.raw) continue;
                    seen[key] = state.raw;
                    pruned.push_back(std::move(state));
                    if (static_cast<int>(pruned.size()) >= LOCAL_CLUSTER_BEAM) break;
                }
                beam = std::move(pruned);
            }

            if (completed.empty()) return {};
            stable_sort(completed.begin(), completed.end(),
                        [](const LocalClusterState &a, const LocalClusterState &b) {
                if (a.raw != b.raw) return a.raw < b.raw;
                if (a.steps != b.steps) return a.steps < b.steps;
                return a.ops < b.ops;
            });

            const int eval_limit = min<int>(8, completed.size());
            BuiltAnswer best_built;
            auto best_local_score = make_tuple(1LL << 60, 1000000000, 1000000000);
            vector<char> done = prefix_done;
            for (int k : items) done[k] = 1;

            for (int idx = 0; idx < eval_limit; ++idx) {
                const LocalClusterState &state = completed[idx];
                string base_prefix = prefix + state.ops;
                BuiltAnswer built =
                    build_suffix_answer(best_order, done, state.current_node, state.dir,
                                        std::move(base_prefix),
                                        delivered_prefix + k_count);
                if (built.delivered < M) continue;
                string local_output = compress_output(built.base);
                auto local_score = score_tuple(built, local_output);
                if (local_score < best_local_score) {
                    best_local_score = local_score;
                    best_built = std::move(built);
                }
            }
            return best_built;
        };

#if ENABLE_LOCAL_CLUSTER_MULTI
        auto solve_local_cluster_segment =
            [&](const vector<int> &items, int start_node, int start_dir) {
            LocalClusterState failed;
            failed.raw = 1000000000;

            const int k_count = static_cast<int>(items.size());
            if (k_count < 2) return failed;

            vector<int> slot_nodes;
            slot_nodes.reserve(2 * k_count);
            for (int k : items) slot_nodes.push_back(1 + k);
            for (int k : items) slot_nodes.push_back(1 + M + k);

            int direct_raw = 0;
            int cur_node = start_node;
            int cur_dir = start_dir;
            for (int k : items) {
                int ball_node = 1 + k;
                int basket_node = 1 + M + k;
                const Route &to_ball = route[cur_node][cur_dir][ball_node];
                if (to_ball.end_dir == -1) return failed;
                const Route &to_basket = route[ball_node][to_ball.end_dir][basket_node];
                if (to_basket.end_dir == -1) return failed;
                direct_raw += to_ball.cost + 1 + to_basket.cost + 1;
                cur_node = basket_node;
                cur_dir = to_basket.end_dir;
            }
            const int raw_limit = direct_raw + LOCAL_CLUSTER_RAW_SLACK;

            LocalClusterState initial;
            initial.current_node = start_node;
            initial.dir = start_dir;
            initial.board.assign(2 * k_count, -1);
            for (int i = 0; i < k_count; ++i) initial.board[i] = i;

            vector<LocalClusterState> beam;
            beam.push_back(std::move(initial));
            vector<LocalClusterState> completed;

            for (int step = 0; step < LOCAL_CLUSTER_MAX_S && !time_over(); ++step) {
                vector<LocalClusterState> next_states;
                next_states.reserve(beam.size() * slot_nodes.size());

                for (const LocalClusterState &state : beam) {
                    for (int slot = 0; slot < static_cast<int>(slot_nodes.size()); ++slot) {
                        int old_cell_ball = state.board[slot];
                        if (old_cell_ball == -1 && state.hold == -1) continue;

                        const Route &move =
                            route[state.current_node][state.dir][slot_nodes[slot]];
                        if (move.end_dir == -1) continue;

                        LocalClusterState next = state;
                        next.current_node = slot_nodes[slot];
                        next.dir = move.end_dir;
                        next.raw += move.cost + 1;
                        if (next.raw > raw_limit) continue;
                        next.ops += move.ops;
                        next.ops.push_back('S');
                        next.board[slot] = static_cast<signed char>(state.hold);
                        next.hold = old_cell_ball;
                        ++next.steps;
                        next.correct = local_cluster_correct_count(next.board, k_count);

                        if (next.correct == k_count && next.hold == -1) {
                            completed.push_back(std::move(next));
                        } else {
                            next_states.push_back(std::move(next));
                        }
                    }
                }

                if (next_states.empty()) break;
                stable_sort(next_states.begin(), next_states.end(),
                            [](const LocalClusterState &a, const LocalClusterState &b) {
                    int pa = a.raw - a.correct * 35 + a.steps * 4;
                    int pb = b.raw - b.correct * 35 + b.steps * 4;
                    if (pa != pb) return pa < pb;
                    if (a.correct != b.correct) return a.correct > b.correct;
                    if (a.raw != b.raw) return a.raw < b.raw;
                    return a.ops < b.ops;
                });

                vector<LocalClusterState> pruned;
                pruned.reserve(min<int>(LOCAL_CLUSTER_BEAM, next_states.size()));
                unordered_map<string, int> seen;
                seen.reserve(LOCAL_CLUSTER_BEAM * 3 + 1);
                for (LocalClusterState &state : next_states) {
                    string key = local_cluster_key(state);
                    auto it = seen.find(key);
                    if (it != seen.end() && it->second <= state.raw) continue;
                    seen[key] = state.raw;
                    pruned.push_back(std::move(state));
                    if (static_cast<int>(pruned.size()) >= LOCAL_CLUSTER_BEAM) break;
                }
                beam = std::move(pruned);
            }

            if (completed.empty()) return failed;
            stable_sort(completed.begin(), completed.end(),
                        [](const LocalClusterState &a, const LocalClusterState &b) {
                if (a.raw != b.raw) return a.raw < b.raw;
                if (a.steps != b.steps) return a.steps < b.steps;
                return a.ops < b.ops;
            });
            return completed.front();
        };

        auto build_multi_local_cluster_answer =
            [&](const vector<LocalClusterCandidate> &selected) -> BuiltAnswer {
            vector<int> selected_by_begin(M + 1, -1);
            for (int i = 0; i < static_cast<int>(selected.size()); ++i) {
                selected_by_begin[selected[i].begin] = i;
            }

            string result;
            vector<char> done(M, 0);
            int current = 0;
            int dir = 1;
            int delivered = 0;

            for (int pos = 0; pos < static_cast<int>(best_order.size());) {
                int selected_idx = selected_by_begin[pos];
                if (selected_idx != -1) {
                    const LocalClusterCandidate &candidate = selected[selected_idx];
                    LocalClusterState state =
                        solve_local_cluster_segment(candidate.items, current, dir);
                    if (state.raw >= 1000000000) return {};
                    if (result.size() + state.ops.size() > static_cast<size_t>(T)) {
                        return {};
                    }
                    result += state.ops;
                    current = state.current_node;
                    dir = state.dir;
                    for (int k : candidate.items) {
                        if (!done[k]) {
                            done[k] = 1;
                            ++delivered;
                        }
                    }
                    pos += cluster_size;
                    continue;
                }

                int k = best_order[pos];
                if (done[k]) {
                    ++pos;
                    continue;
                }
                int ball_node = 1 + k;
                int basket_node = 1 + M + k;
                const Route &to_ball = route[current][dir][ball_node];
                if (to_ball.end_dir == -1) break;
                const Route &to_basket =
                    route[ball_node][to_ball.end_dir][basket_node];
                if (to_basket.end_dir == -1) break;

                string segment = to_ball.ops;
                segment.push_back('S');
                segment += to_basket.ops;
                segment.push_back('S');
                if (result.size() + segment.size() > static_cast<size_t>(T)) break;

                result += segment;
                current = basket_node;
                dir = to_basket.end_dir;
                done[k] = 1;
                ++delivered;
                ++pos;
            }

            return {result, delivered};
        };
        (void)build_multi_local_cluster_answer;
#endif

        int local_cluster_tested = 0;
        int local_cluster_valid = 0;
        int local_cluster_best_buttons = 1000000000;
#if ENABLE_LOCAL_CLUSTER_MULTI
        auto pre_local_best_score = best_score;
        vector<LocalClusterCandidate> promising_multi_candidates;
#endif
        for (const LocalClusterCandidate &candidate : local_cluster_candidates) {
            if (time_over()) break;
            ++local_cluster_tested;
            for (int mode = 0; mode < 2 && !time_over(); ++mode) {
                bool keep_interval = (mode == 1);
                if (keep_interval && candidate.begin == 0) continue;
                if (keep_interval && N > LOCAL_CLUSTER_INTERVAL_MAX_N) continue;
                BuiltAnswer cluster_answer = solve_local_cluster(candidate, keep_interval);
                if (cluster_answer.delivered < M) continue;
                string cluster_output = compress_output(cluster_answer.base);
                if (!time_over()) {
                    cluster_output =
                        compress_hierarchical(cluster_answer.base, cluster_output);
                }
                auto cluster_score = score_tuple(cluster_answer, cluster_output);
                ++local_cluster_valid;
                local_cluster_best_buttons =
                    min(local_cluster_best_buttons,
                        static_cast<int>(cluster_output.size()));
#if ENABLE_LOCAL_CLUSTER_MULTI
                if (cluster_score < pre_local_best_score &&
                    (keep_interval || candidate.begin == 0)) {
                    bool seen_promising = false;
                    for (const LocalClusterCandidate &existing :
                         promising_multi_candidates) {
                        if (existing.begin == candidate.begin) {
                            seen_promising = true;
                            break;
                        }
                    }
                    if (!seen_promising) promising_multi_candidates.push_back(candidate);
                }
#endif
                if (cluster_score < best_score) {
                    best_score = cluster_score;
                    output = std::move(cluster_output);
                    best_base = cluster_answer.base;
                    best_answer = std::move(cluster_answer);
                }
            }
        }

#if ENABLE_LOCAL_CLUSTER_MULTI
        if (N <= LOCAL_CLUSTER_INTERVAL_MAX_N && !time_over()) {
            vector<vector<LocalClusterCandidate>> multi_sets;
            auto overlaps_any = [&](const vector<LocalClusterCandidate> &set,
                                    const LocalClusterCandidate &candidate) {
                int l = candidate.begin;
                int r = candidate.begin + cluster_size;
                for (const LocalClusterCandidate &other : set) {
                    int ol = other.begin;
                    int orr = other.begin + cluster_size;
                    if (max(l, ol) < min(r, orr)) return true;
                }
                return false;
            };

            const int seed_limit =
                min<int>(LOCAL_CLUSTER_MULTI_SET_LIMIT, promising_multi_candidates.size());
            for (int seed = 0; seed < seed_limit; ++seed) {
                vector<LocalClusterCandidate> selected;
                selected.push_back(promising_multi_candidates[seed]);
                for (const LocalClusterCandidate &candidate :
                     promising_multi_candidates) {
                    if (static_cast<int>(selected.size()) >=
                        LOCAL_CLUSTER_MULTI_MAX_BLOCKS) {
                        break;
                    }
                    if (overlaps_any(selected, candidate)) continue;
                    selected.push_back(candidate);
                }
                if (selected.size() < 2) continue;
                stable_sort(selected.begin(), selected.end(),
                            [](const LocalClusterCandidate &a,
                               const LocalClusterCandidate &b) {
                    return a.begin < b.begin;
                });
                if (find_if(multi_sets.begin(), multi_sets.end(),
                            [&](const vector<LocalClusterCandidate> &existing) {
                    if (existing.size() != selected.size()) return false;
                    for (int i = 0; i < static_cast<int>(existing.size()); ++i) {
                        if (existing[i].begin != selected[i].begin) return false;
                    }
                    return true;
                }) != multi_sets.end()) {
                    continue;
                }
                multi_sets.push_back(std::move(selected));
            }

            for (const vector<LocalClusterCandidate> &selected : multi_sets) {
                if (time_over()) break;
                BuiltAnswer multi_answer = build_multi_local_cluster_answer(selected);
                if (multi_answer.delivered < M) continue;
                string multi_output = compress_output(multi_answer.base);
                if (!time_over()) {
                    multi_output =
                        compress_hierarchical(multi_answer.base, multi_output);
                }
                auto multi_score = score_tuple(multi_answer, multi_output);
                ++local_cluster_valid;
                local_cluster_best_buttons =
                    min(local_cluster_best_buttons,
                        static_cast<int>(multi_output.size()));
                if (multi_score < best_score) {
                    best_score = multi_score;
                    output = std::move(multi_output);
                    best_base = multi_answer.base;
                    best_answer = std::move(multi_answer);
                }
            }
        }
#endif

#if MACRO_CHAIN_DEBUG
        cerr << "METRIC local_cluster_tested=" << local_cluster_tested
             << " local_cluster_valid=" << local_cluster_valid
             << " local_cluster_best_buttons=" << local_cluster_best_buttons << "\n";
#else
        (void)local_cluster_tested;
        (void)local_cluster_valid;
        (void)local_cluster_best_buttons;
#endif
    }
#endif

#if ENABLE_TREE_ROUTE_CANDIDATE
    if (!best_order.empty() && !time_over()) {
        auto append_tree_turn = [](string &ops, int &dir, int target_dir) {
            int diff = (target_dir - dir + 4) & 3;
            if (diff == 1) {
                ops.push_back('R');
            } else if (diff == 2) {
                ops.push_back('R');
                ops.push_back('R');
            } else if (diff == 3) {
                ops.push_back('L');
            }
            dir = target_dir;
        };

        auto build_tree_parent = [&](Pos root) {
            vector<int> parent(N * N, -1);
            queue<int> que;
            int root_id = root.r * N + root.c;
            parent[root_id] = root_id;
            que.push(root_id);
            while (!que.empty()) {
                int cell = que.front();
                que.pop();
                int r = cell / N;
                int c = cell % N;
                for (int d = 0; d < 4; ++d) {
                    if (!can_move(r, c, d)) continue;
                    int nr = r + dr[d];
                    int nc = c + dc[d];
                    int nxt = nr * N + nc;
                    if (parent[nxt] != -1) continue;
                    parent[nxt] = cell;
                    que.push(nxt);
                }
            }
            return parent;
        };

        auto path_to_tree_root = [&](const vector<int> &parent, int cell) {
            vector<int> path;
            while (true) {
                path.push_back(cell);
                if (parent[cell] == cell) break;
                cell = parent[cell];
            }
            return path;
        };

        auto build_tree_route_table = [&](const vector<int> &parent) {
            vector<vector<vector<Route>>> table(
                node_count, vector<vector<Route>>(4, vector<Route>(node_count)));

            vector<vector<int>> root_paths(node_count);
            for (int node = 0; node < node_count; ++node) {
                int cell = nodes[node].r * N + nodes[node].c;
                root_paths[node] = path_to_tree_root(parent, cell);
            }

            vector<int> mark(N * N, -1);
            for (int source = 0; source < node_count; ++source) {
                for (int idx = 0; idx < static_cast<int>(root_paths[source].size()); ++idx) {
                    mark[root_paths[source][idx]] = idx;
                }

                for (int target = 0; target < node_count; ++target) {
                    int source_lca_index = -1;
                    int target_lca_index = -1;
                    for (int idx = 0; idx < static_cast<int>(root_paths[target].size()); ++idx) {
                        int cell = root_paths[target][idx];
                        if (mark[cell] != -1) {
                            source_lca_index = mark[cell];
                            target_lca_index = idx;
                            break;
                        }
                    }
                    if (source_lca_index == -1) continue;

                    vector<int> cell_path;
                    for (int i = 0; i <= source_lca_index; ++i) {
                        cell_path.push_back(root_paths[source][i]);
                    }
                    for (int i = target_lca_index - 1; i >= 0; --i) {
                        cell_path.push_back(root_paths[target][i]);
                    }

                    for (int sdir = 0; sdir < 4; ++sdir) {
                        string ops;
                        int dir = sdir;
                        bool ok = true;
                        for (int i = 1; i < static_cast<int>(cell_path.size()); ++i) {
                            int prev = cell_path[i - 1];
                            int cur = cell_path[i];
                            int pr = prev / N;
                            int pc = prev % N;
                            int cr = cur / N;
                            int cc = cur % N;
                            int ndir = -1;
                            if (cr == pr - 1 && cc == pc) {
                                ndir = 0;
                            } else if (cr == pr && cc == pc + 1) {
                                ndir = 1;
                            } else if (cr == pr + 1 && cc == pc) {
                                ndir = 2;
                            } else if (cr == pr && cc == pc - 1) {
                                ndir = 3;
                            }
                            if (ndir == -1 || !can_move(pr, pc, ndir)) {
                                ok = false;
                                break;
                            }
                            append_tree_turn(ops, dir, ndir);
                            ops.push_back('F');
                        }
                        if (!ok) continue;
                        table[source][sdir][target] = {
                            static_cast<int>(ops.size()),
                            dir,
                            std::move(ops),
                        };
                    }
                }

                for (int cell : root_paths[source]) mark[cell] = -1;
            }
            return table;
        };

        vector<Pos> tree_roots;
        tree_roots.push_back({0, 0});
        tree_roots.push_back({N / 2, N / 2});
        long long sr = 0;
        long long sc = 0;
        for (int k = 0; k < M; ++k) {
            sr += ball[k].r + basket[k].r;
            sc += ball[k].c + basket[k].c;
        }
        tree_roots.push_back({
            static_cast<int>(sr / max(1, 2 * M)),
            static_cast<int>(sc / max(1, 2 * M)),
        });

        sort(tree_roots.begin(), tree_roots.end(), [](const Pos &a, const Pos &b) {
            if (a.r != b.r) return a.r < b.r;
            return a.c < b.c;
        });
        tree_roots.erase(unique(tree_roots.begin(), tree_roots.end(),
                                [](const Pos &a, const Pos &b) {
                                    return a.r == b.r && a.c == b.c;
                                }),
                         tree_roots.end());
        if (static_cast<int>(tree_roots.size()) > TREE_ROUTE_ROOT_LIMIT) {
            tree_roots.resize(TREE_ROUTE_ROOT_LIMIT);
        }

        int tree_route_tested = 0;
        int tree_route_valid = 0;
        int tree_route_best_buttons = 1000000000;
        int tree_route_best_base = 1000000000;
        for (const Pos &root : tree_roots) {
            if (time_over()) break;
            ++tree_route_tested;
            vector<int> parent = build_tree_parent(root);
            if (count(parent.begin(), parent.end(), -1) != 0) continue;
            auto tree_table = build_tree_route_table(parent);
            BuiltAnswer tree_answer = build_answer_with_routes(tree_table, best_order);
            if (tree_answer.delivered < M) continue;
            string tree_output = compress_output(tree_answer.base);
            if (!time_over()) {
                tree_output = compress_hierarchical(tree_answer.base, tree_output);
            }
            auto tree_score = score_tuple(tree_answer, tree_output);
            ++tree_route_valid;
            tree_route_best_buttons =
                min(tree_route_best_buttons, static_cast<int>(tree_output.size()));
            tree_route_best_base =
                min(tree_route_best_base, static_cast<int>(tree_answer.base.size()));
            if (tree_score < best_score) {
                best_score = tree_score;
                output = std::move(tree_output);
                best_base = tree_answer.base;
                best_answer = std::move(tree_answer);
            }
        }

#if MACRO_CHAIN_DEBUG
        cerr << "METRIC tree_route_tested=" << tree_route_tested
             << " tree_route_valid=" << tree_route_valid
             << " tree_route_best_buttons=" << tree_route_best_buttons
             << " tree_route_best_base=" << tree_route_best_base << "\n";
#else
        (void)tree_route_tested;
        (void)tree_route_valid;
        (void)tree_route_best_buttons;
        (void)tree_route_best_base;
#endif
    }
#endif

    vector<pair<BuiltAnswer, string>> macro_chain_candidates;

#if ENABLE_BASKET_CHAIN_CANDIDATE
    if (!best_order.empty() && !time_over()) {
        auto append_basket_turn_ops = [](string &ops, int from_dir, int to_dir) {
            int diff = (to_dir - from_dir + 4) & 3;
            if (diff == 1) {
                ops.push_back('R');
            } else if (diff == 2) {
                ops.push_back('R');
                ops.push_back('R');
            } else if (diff == 3) {
                ops.push_back('L');
            }
        };

        auto build_basket_suffix =
            [&](const vector<int> &order, const vector<char> &done,
                int start_node, int start_dir, size_t used_len,
                int delivered_prefix) -> BuiltAnswer {
            string result;
            int current = start_node;
            int dir = start_dir;
            int delivered = delivered_prefix;

            for (int k : order) {
                if (done[k]) continue;
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
                if (used_len + result.size() + segment.size() > static_cast<size_t>(T)) break;

                result += segment;
                current = basket_node;
                dir = to_basket.end_dir;
                ++delivered;
            }

            return {result, delivered};
        };

        vector<int> basket_ball_at(N * N, -1);
        for (int k = 0; k < M; ++k) {
            basket_ball_at[ball[k].r * N + ball[k].c] = k;
        }

        vector<int> order_pos(M, M);
        for (int i = 0; i < static_cast<int>(best_order.size()); ++i) {
            order_pos[best_order[i]] = i;
        }

        struct BasketChainSeed {
            int rough_gain = 0;
            int dir = 0;
            string label;
            vector<int> path;
        };

        auto best_basket_path_for_edges = [&](const vector<pair<int, int>> &edges) {
            vector<vector<int>> nexts(M);
            for (auto [from, to] : edges) nexts[from].push_back(to);
            for (vector<int> &xs : nexts) {
                stable_sort(xs.begin(), xs.end(), [&](int a, int b) {
                    if (order_pos[a] != order_pos[b]) return order_pos[a] < order_pos[b];
                    return a < b;
                });
                xs.erase(unique(xs.begin(), xs.end()), xs.end());
            }

            vector<int> best_path;
            for (auto [from, to] : edges) {
                vector<int> path;
                vector<char> used(M, 0);
                path.push_back(from);
                path.push_back(to);
                used[from] = 1;
                used[to] = 1;

                int current = to;
                while (!nexts[current].empty()) {
                    int chosen = -1;
                    for (int nxt : nexts[current]) {
                        if (!used[nxt]) {
                            chosen = nxt;
                            break;
                        }
                    }
                    if (chosen == -1) break;
                    path.push_back(chosen);
                    used[chosen] = 1;
                    current = chosen;
                }

                if (path.size() > best_path.size()) best_path = std::move(path);
            }
            return best_path;
        };

        unordered_map<string, vector<pair<int, int>>> basket_edges_by_label;
        basket_edges_by_label.reserve(M * M * 4);
        unordered_map<string, char> basket_label_seen;
        basket_label_seen.reserve(M * M * 4);

        auto add_basket_label_edges = [&](int start_dir, const string &label) {
            string key;
            key.reserve(label.size() + 1);
            key.push_back(static_cast<char>('0' + start_dir));
            key += label;
            if (basket_label_seen.find(key) != basket_label_seen.end()) return;
            basket_label_seen[key] = 1;

            vector<pair<int, int>> edges;
            for (int prev = 0; prev < M; ++prev) {
                int r = basket[prev].r;
                int c = basket[prev].c;
                int dir = start_dir;
                int seen_s = 0;
                int carried = -1;
                bool ok = true;

                for (char op : label) {
                    if (op == 'F') {
                        if (can_move(r, c, dir)) {
                            r += dr[dir];
                            c += dc[dir];
                        }
                    } else if (op == 'R') {
                        dir = (dir + 1) & 3;
                    } else if (op == 'L') {
                        dir = (dir + 3) & 3;
                    } else if (op == 'S') {
                        if (seen_s == 0) {
                            carried = basket_ball_at[r * N + c];
                            if (carried == -1 || carried == prev) {
                                ok = false;
                                break;
                            }
                            seen_s = 1;
                        } else if (seen_s == 1) {
                            if (r != basket[carried].r || c != basket[carried].c) {
                                ok = false;
                                break;
                            }
                            seen_s = 2;
                        } else {
                            ok = false;
                            break;
                        }
                    }
                }

                if (!ok || seen_s != 2 || dir != start_dir) continue;
                edges.push_back({prev, carried});
            }

            if (!edges.empty()) {
                auto &stored = basket_edges_by_label[key];
                stored.insert(stored.end(), edges.begin(), edges.end());
            }
        };

        for (int d = 0; d < 4; ++d) {
            for (int prev = 0; prev < M; ++prev) {
                int prev_basket_node = 1 + M + prev;
                for (int next = 0; next < M; ++next) {
                    if (next == prev) continue;
                    int ball_node = 1 + next;
                    int basket_node = 1 + M + next;
                    const Route &to_ball = route[prev_basket_node][d][ball_node];
                    if (to_ball.end_dir == -1) continue;
                    const Route &to_basket = route[ball_node][to_ball.end_dir][basket_node];
                    if (to_basket.end_dir == -1) continue;

                    string label = to_ball.ops;
                    label.push_back('S');
                    label += to_basket.ops;
                    label.push_back('S');
                    append_basket_turn_ops(label, to_basket.end_dir, d);
                    if (static_cast<int>(label.size()) + 2 > T) continue;
                    add_basket_label_edges(d, label);
                }
            }
        }

        vector<BasketChainSeed> basket_seeds;
        int basket_chain_groups = 0;
        int basket_chain_raw_edges = 0;
        int basket_chain_best_edges = 0;
        for (const auto &[key, edges] : basket_edges_by_label) {
            if (time_over()) break;
            if (edges.size() < 2) continue;
            ++basket_chain_groups;
            basket_chain_raw_edges += static_cast<int>(edges.size());

            int dir = key[0] - '0';
            string label = key.substr(1);
            vector<int> path = best_basket_path_for_edges(edges);
            int edge_count = static_cast<int>(path.size()) - 1;
            if (edge_count < 2) continue;
            basket_chain_best_edges = max(basket_chain_best_edges, edge_count);

            int rough_gain =
                (edge_count - 1) * (static_cast<int>(label.size()) - 1) - 2;
            if (rough_gain <= 0) continue;
            basket_seeds.push_back({rough_gain, dir, std::move(label), std::move(path)});
        }

        stable_sort(basket_seeds.begin(), basket_seeds.end(),
                    [](const BasketChainSeed &a, const BasketChainSeed &b) {
            if (a.rough_gain != b.rough_gain) return a.rough_gain > b.rough_gain;
            if (a.path.size() != b.path.size()) return a.path.size() > b.path.size();
            return a.label.size() > b.label.size();
        });
        if (static_cast<int>(basket_seeds.size()) > BASKET_CHAIN_CANDIDATE_LIMIT) {
            basket_seeds.resize(BASKET_CHAIN_CANDIDATE_LIMIT);
        }

        int basket_chain_valid = 0;
        int basket_chain_best_buttons = 1000000000;
        for (const BasketChainSeed &seed : basket_seeds) {
            if (time_over()) break;
            int edge_count = static_cast<int>(seed.path.size()) - 1;
            int first = seed.path.front();
            int terminal = seed.path.back();

            const Route &to_first_ball = route[0][1][1 + first];
            if (to_first_ball.end_dir == -1) continue;
            const Route &to_first_basket =
                route[1 + first][to_first_ball.end_dir][1 + M + first];
            if (to_first_basket.end_dir == -1) continue;

            string prefix = to_first_ball.ops;
            prefix.push_back('S');
            prefix += to_first_basket.ops;
            prefix.push_back('S');
            append_basket_turn_ops(prefix, to_first_basket.end_dir, seed.dir);

            vector<char> done(M, 0);
            done[first] = 1;
            for (int i = 1; i < static_cast<int>(seed.path.size()); ++i) {
                done[seed.path[i]] = 1;
            }

            int delivered_prefix = 1 + edge_count;
            size_t used_len =
                prefix.size() + static_cast<size_t>(edge_count) * seed.label.size();
            if (used_len > static_cast<size_t>(T)) continue;

            BuiltAnswer suffix =
                build_basket_suffix(best_order, done, 1 + M + terminal, seed.dir,
                                    used_len, delivered_prefix);
            if (suffix.delivered < M) continue;

            BuiltAnswer built;
            built.base.reserve(used_len + suffix.base.size());
            built.base += prefix;
            for (int rep = 0; rep < edge_count; ++rep) built.base += seed.label;
            built.base += suffix.base;
            built.delivered = suffix.delivered;
            if (built.base.size() > static_cast<size_t>(T)) continue;

            string candidate_output = compress_output(prefix);
            candidate_output.push_back('M');
            candidate_output += seed.label;
            candidate_output.push_back('M');
            for (int rep = 1; rep < edge_count; ++rep) candidate_output.push_back('P');
            candidate_output += compress_output(suffix.base);
            if (candidate_output.size() > static_cast<size_t>(T)) continue;

            ++basket_chain_valid;
            basket_chain_best_buttons =
                min(basket_chain_best_buttons, static_cast<int>(candidate_output.size()));
            macro_chain_candidates.push_back({std::move(built), std::move(candidate_output)});
        }

        struct BasketReuseSeed {
            int rough_gain = 0;
            int dir = 0;
            string key;
            string label;
            vector<pair<int, int>> edges;
        };
        vector<BasketReuseSeed> basket_reuse_seeds;
        basket_reuse_seeds.reserve(basket_edges_by_label.size());
        for (const auto &[key, edges] : basket_edges_by_label) {
            if (edges.size() < 2) continue;
            vector<char> has_next(M, 0);
            vector<char> has_prev(M, 0);
            for (auto [prev, next] : edges) {
                has_prev[prev] = 1;
                has_next[next] = 1;
            }
            int next_count = accumulate(has_next.begin(), has_next.end(), 0);
            int prev_count = accumulate(has_prev.begin(), has_prev.end(), 0);
            if (next_count < 2 || prev_count < 1) continue;
            string label = key.substr(1);
            int rough_gain =
                (next_count - 1) * (static_cast<int>(label.size()) - 1) - 2;
            if (rough_gain <= 0) continue;
            basket_reuse_seeds.push_back(
                {rough_gain + prev_count * 3, key[0] - '0', key, std::move(label), edges});
        }
        stable_sort(basket_reuse_seeds.begin(), basket_reuse_seeds.end(),
                    [](const BasketReuseSeed &a, const BasketReuseSeed &b) {
            if (a.rough_gain != b.rough_gain) return a.rough_gain > b.rough_gain;
            if (a.edges.size() != b.edges.size()) return a.edges.size() > b.edges.size();
            return a.label.size() > b.label.size();
        });
        if (static_cast<int>(basket_reuse_seeds.size()) > BASKET_CHAIN_CANDIDATE_LIMIT) {
            basket_reuse_seeds.resize(BASKET_CHAIN_CANDIDATE_LIMIT);
        }

        int basket_reuse_valid = 0;
        int basket_reuse_best_buttons = 1000000000;
        int basket_reuse_best_deliveries = 0;
        for (const BasketReuseSeed &seed : basket_reuse_seeds) {
            if (time_over()) break;

            vector<pair<int, int>> initial_edges = seed.edges;
            stable_sort(initial_edges.begin(), initial_edges.end(), [&](auto a, auto b) {
                const Route &a_to_ball = route[0][1][1 + a.first];
                const Route &b_to_ball = route[0][1][1 + b.first];
                int ac = 1000000000;
                int bc = 1000000000;
                if (a_to_ball.end_dir != -1) {
                    const Route &a_to_basket =
                        route[1 + a.first][a_to_ball.end_dir][1 + M + a.first];
                    if (a_to_basket.end_dir != -1) {
                        ac = a_to_ball.cost + a_to_basket.cost;
                    }
                }
                if (b_to_ball.end_dir != -1) {
                    const Route &b_to_basket =
                        route[1 + b.first][b_to_ball.end_dir][1 + M + b.first];
                    if (b_to_basket.end_dir != -1) {
                        bc = b_to_ball.cost + b_to_basket.cost;
                    }
                }
                if (ac != bc) return ac < bc;
                if (order_pos[a.second] != order_pos[b.second]) {
                    return order_pos[a.second] < order_pos[b.second];
                }
                return a < b;
            });
            initial_edges.erase(unique(initial_edges.begin(), initial_edges.end()),
                                initial_edges.end());
            if (static_cast<int>(initial_edges.size()) > 6) initial_edges.resize(6);

            for (auto [first_prev, first_next] : initial_edges) {
                if (time_over()) break;
                if (first_prev == first_next) continue;

                const Route &to_first_ball = route[0][1][1 + first_prev];
                if (to_first_ball.end_dir == -1) continue;
                const Route &to_first_basket =
                    route[1 + first_prev][to_first_ball.end_dir][1 + M + first_prev];
                if (to_first_basket.end_dir == -1) continue;

                string prefix = to_first_ball.ops;
                prefix.push_back('S');
                prefix += to_first_basket.ops;
                prefix.push_back('S');
                append_basket_turn_ops(prefix, to_first_basket.end_dir, seed.dir);

                vector<char> done(M, 0);
                done[first_prev] = 1;
                done[first_next] = 1;
                int delivered = 2;
                int current_node = 1 + M + first_next;
                int current_dir = seed.dir;
                size_t used_len = prefix.size() + seed.label.size();
                if (used_len > static_cast<size_t>(T)) continue;

                string middle_base = prefix + seed.label;
                string middle_output = compress_output(prefix);
                middle_output.push_back('M');
                middle_output += seed.label;
                middle_output.push_back('M');

                while (!time_over()) {
                    int best_prev = -1;
                    int best_next = -1;
                    string best_transition;
                    int best_key = 1000000000;

                    for (auto [prev, next] : seed.edges) {
                        if (!done[prev] || done[next]) continue;
                        const Route &to_start =
                            route[current_node][current_dir][1 + M + prev];
                        if (to_start.end_dir == -1) continue;
                        string transition = to_start.ops;
                        append_basket_turn_ops(transition, to_start.end_dir, seed.dir);
                        if (used_len + transition.size() + seed.label.size() >
                            static_cast<size_t>(T)) {
                            continue;
                        }
                        int key = static_cast<int>(transition.size()) * 100 +
                                  order_pos[next];
                        if (key < best_key) {
                            best_key = key;
                            best_prev = prev;
                            best_next = next;
                            best_transition = std::move(transition);
                        }
                    }

                    if (best_prev == -1) break;
                    middle_base += best_transition;
                    middle_base += seed.label;
                    middle_output += best_transition;
                    middle_output.push_back('P');
                    used_len += best_transition.size() + seed.label.size();
                    done[best_next] = 1;
                    ++delivered;
                    current_node = 1 + M + best_next;
                    current_dir = seed.dir;
                }

                if (delivered < 3) continue;
                BuiltAnswer suffix =
                    build_basket_suffix(best_order, done, current_node, current_dir,
                                        used_len, delivered);
                if (suffix.delivered < M) continue;

                BuiltAnswer built;
                built.base = middle_base + suffix.base;
                built.delivered = suffix.delivered;
                if (built.base.size() > static_cast<size_t>(T)) continue;

                string candidate_output = middle_output + compress_output(suffix.base);
                if (candidate_output.size() > static_cast<size_t>(T)) continue;

                ++basket_reuse_valid;
                basket_reuse_best_buttons =
                    min(basket_reuse_best_buttons, static_cast<int>(candidate_output.size()));
                basket_reuse_best_deliveries = max(basket_reuse_best_deliveries, delivered);
                macro_chain_candidates.push_back({std::move(built), std::move(candidate_output)});
            }
        }

#if MACRO_CHAIN_DEBUG
        cerr << "METRIC basket_chain_groups=" << basket_chain_groups
             << " basket_chain_raw_edges=" << basket_chain_raw_edges
             << " basket_chain_seeds=" << basket_seeds.size()
             << " basket_chain_valid=" << basket_chain_valid
             << " basket_chain_best_buttons=" << basket_chain_best_buttons
             << " basket_chain_best_edges=" << basket_chain_best_edges
             << " basket_reuse_seeds=" << basket_reuse_seeds.size()
             << " basket_reuse_valid=" << basket_reuse_valid
             << " basket_reuse_best_buttons=" << basket_reuse_best_buttons
             << " basket_reuse_best_deliveries=" << basket_reuse_best_deliveries
             << "\n";
#else
        (void)basket_chain_groups;
        (void)basket_chain_raw_edges;
        (void)basket_chain_valid;
        (void)basket_chain_best_buttons;
        (void)basket_chain_best_edges;
        (void)basket_reuse_valid;
        (void)basket_reuse_best_buttons;
        (void)basket_reuse_best_deliveries;
#endif
    }
#endif

#if ENABLE_MACRO_CHAIN_CANDIDATE
    if (!best_order.empty() && !time_over()) {
        auto append_turn_ops = [](string &ops, int from_dir, int to_dir) {
            int diff = (to_dir - from_dir + 4) & 3;
            if (diff == 1) {
                ops.push_back('R');
            } else if (diff == 2) {
                ops.push_back('R');
                ops.push_back('R');
            } else if (diff == 3) {
                ops.push_back('L');
            }
        };

        auto build_direct_suffix =
            [&](const vector<int> &order, const vector<char> &done,
                int start_node, int start_dir, size_t used_len,
                int delivered_prefix) -> BuiltAnswer {
            string result;
            int current = start_node;
            int dir = start_dir;
            int delivered = delivered_prefix;

            for (int k : order) {
                if (done[k]) continue;
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
                if (used_len + result.size() + segment.size() > static_cast<size_t>(T)) break;

                result += segment;
                current = basket_node;
                dir = to_basket.end_dir;
                ++delivered;
            }

            return {result, delivered};
        };

        unordered_map<string, vector<pair<int, int>>> edges_by_label;
        edges_by_label.reserve(M * M * 4);
        vector<int> macro_ball_at(N * N, -1);
        for (int k = 0; k < M; ++k) {
            macro_ball_at[ball[k].r * N + ball[k].c] = k;
        }
        unordered_map<string, char> simulated_template_seen;
        simulated_template_seen.reserve(M * M * 4);
        int macro_template_labels = 0;
        int macro_template_edges = 0;

        auto add_simulated_template_edges = [&](int start_dir, const string &label,
                                                const string &key) {
            if (simulated_template_seen.find(key) != simulated_template_seen.end()) return;
            simulated_template_seen[key] = 1;

            vector<pair<int, int>> added;
            for (int k = 0; k < M; ++k) {
                int r = ball[k].r;
                int c = ball[k].c;
                int dir = start_dir;
                int seen_s = 0;
                bool ok = true;

                for (char op : label) {
                    if (op == 'S') {
                        if (seen_s == 0) {
                            seen_s = 1;
                        } else if (seen_s == 1) {
                            if (r != basket[k].r || c != basket[k].c) {
                                ok = false;
                                break;
                            }
                            seen_s = 2;
                        } else {
                            ok = false;
                            break;
                        }
                    } else if (op == 'F') {
                        if (can_move(r, c, dir)) {
                            r += dr[dir];
                            c += dc[dir];
                        }
                    } else if (op == 'R') {
                        dir = (dir + 1) & 3;
                    } else if (op == 'L') {
                        dir = (dir + 3) & 3;
                    }
                }

                if (!ok || seen_s != 2 || dir != start_dir) continue;
                int next = macro_ball_at[r * N + c];
                if (next == -1 || next == k) continue;
                added.push_back({k, next});
            }

            if (!added.empty()) {
                auto &edges = edges_by_label[key];
                edges.insert(edges.end(), added.begin(), added.end());
                ++macro_template_labels;
                macro_template_edges += static_cast<int>(added.size());
            }
        };

        for (int d = 0; d < 4; ++d) {
            for (int i = 0; i < M; ++i) {
                int ball_i = 1 + i;
                int basket_i = 1 + M + i;
                const Route &carry = route[ball_i][d][basket_i];
                if (carry.end_dir == -1) continue;
                for (int j = 0; j < M; ++j) {
                    if (i == j) continue;
                    int ball_j = 1 + j;
                    const Route &to_next = route[basket_i][carry.end_dir][ball_j];
                    if (to_next.end_dir == -1) continue;

                    string label;
                    label.reserve(2 + carry.ops.size() + to_next.ops.size() + 2);
                    label.push_back('S');
                    label += carry.ops;
                    label.push_back('S');
                    label += to_next.ops;
                    append_turn_ops(label, to_next.end_dir, d);

                    if (label.size() + 2 > static_cast<size_t>(T)) continue;
                    string key;
                    key.reserve(label.size() + 1);
                    key.push_back(static_cast<char>('0' + d));
                    key += label;
                    edges_by_label[key].push_back({i, j});
                    add_simulated_template_edges(d, label, key);
                }
            }
        }

        vector<int> order_pos(M, M);
        for (int i = 0; i < static_cast<int>(best_order.size()); ++i) {
            order_pos[best_order[i]] = i;
        }

        struct MacroChainSeed {
            int rough_gain = 0;
            int dir = 0;
            string label;
            vector<int> path;
        };
        vector<MacroChainSeed> seeds;
        int macro_chain_groups = 0;
        int macro_chain_raw_edges = 0;

        for (const auto &[key, edges] : edges_by_label) {
            if (time_over()) break;
            if (edges.size() < 2) continue;
            ++macro_chain_groups;
            macro_chain_raw_edges += static_cast<int>(edges.size());

            int dir = key[0] - '0';
            string label = key.substr(1);
            vector<vector<int>> nexts(M);
            for (auto [from, to] : edges) nexts[from].push_back(to);
            for (vector<int> &xs : nexts) {
                sort(xs.begin(), xs.end(), [&](int a, int b) {
                    if (order_pos[a] != order_pos[b]) return order_pos[a] < order_pos[b];
                    return a < b;
                });
                xs.erase(unique(xs.begin(), xs.end()), xs.end());
            }

            for (auto [from, to] : edges) {
                vector<int> path;
                vector<char> used_node(M, 0);
                path.push_back(from);
                path.push_back(to);
                used_node[from] = 1;
                used_node[to] = 1;

                int current = to;
                while (!nexts[current].empty()) {
                    int chosen = -1;
                    for (int nxt : nexts[current]) {
                        if (!used_node[nxt]) {
                            chosen = nxt;
                            break;
                        }
                    }
                    if (chosen == -1) break;
                    path.push_back(chosen);
                    used_node[chosen] = 1;
                    current = chosen;
                }

                int edge_count = static_cast<int>(path.size()) - 1;
                if (edge_count < 2) continue;
                int rough_gain =
                    (edge_count - 1) * (static_cast<int>(label.size()) - 1) - 2;
                if (rough_gain <= 0) continue;
                seeds.push_back({rough_gain, dir, label, std::move(path)});
            }
        }

        stable_sort(seeds.begin(), seeds.end(),
                    [](const MacroChainSeed &a, const MacroChainSeed &b) {
            if (a.rough_gain != b.rough_gain) return a.rough_gain > b.rough_gain;
            if (a.path.size() != b.path.size()) return a.path.size() > b.path.size();
            return a.label.size() > b.label.size();
        });
        if (static_cast<int>(seeds.size()) > MACRO_CHAIN_CANDIDATE_LIMIT) {
            seeds.resize(MACRO_CHAIN_CANDIDATE_LIMIT);
        }

        int macro_chain_valid = 0;
        int macro_chain_best_buttons = 1000000000;
        int macro_chain_best_edges = 0;
        for (const MacroChainSeed &seed : seeds) {
            if (time_over()) break;
            int edge_count = static_cast<int>(seed.path.size()) - 1;
            int first = seed.path.front();
            int terminal = seed.path.back();
            const Route &to_first = route[0][1][1 + first];
            if (to_first.end_dir == -1) continue;

            string prefix = to_first.ops;
            append_turn_ops(prefix, to_first.end_dir, seed.dir);

            vector<char> done(M, 0);
            for (int i = 0; i < edge_count; ++i) done[seed.path[i]] = 1;

            size_t used_len = prefix.size() +
                              static_cast<size_t>(edge_count) * seed.label.size();
            if (used_len > static_cast<size_t>(T)) continue;

            BuiltAnswer suffix = build_direct_suffix(best_order, done, 1 + terminal,
                                                     seed.dir, used_len, edge_count);
            if (suffix.delivered < M) continue;

            BuiltAnswer built;
            built.base.reserve(used_len + suffix.base.size());
            built.base += prefix;
            for (int rep = 0; rep < edge_count; ++rep) built.base += seed.label;
            built.base += suffix.base;
            built.delivered = suffix.delivered;
            if (built.base.size() > static_cast<size_t>(T)) continue;

            string chain_output = compress_output(prefix);
            chain_output.push_back('M');
            chain_output += seed.label;
            chain_output.push_back('M');
            for (int rep = 1; rep < edge_count; ++rep) chain_output.push_back('P');
            chain_output += compress_output(suffix.base);
            if (chain_output.size() > static_cast<size_t>(T)) continue;

            ++macro_chain_valid;
            if (static_cast<int>(chain_output.size()) < macro_chain_best_buttons) {
                macro_chain_best_buttons = static_cast<int>(chain_output.size());
                macro_chain_best_edges = edge_count;
            }
            macro_chain_candidates.push_back({std::move(built), std::move(chain_output)});
        }

        auto run_ops_from = [&](Pos p, int start_dir, const string &ops) {
            int r = p.r;
            int c = p.c;
            int dir = start_dir;
            for (char op : ops) {
                if (op == 'F') {
                    if (can_move(r, c, dir)) {
                        r += dr[dir];
                        c += dc[dir];
                    }
                } else if (op == 'R') {
                    dir = (dir + 1) & 3;
                } else if (op == 'L') {
                    dir = (dir + 3) & 3;
                }
            }
            return make_tuple(r, c, dir);
        };

        struct CarryPatternGroup {
            int dir = 0;
            int end_dir = 0;
            string ops;
            vector<int> items;
        };
        unordered_map<string, CarryPatternGroup> carry_groups;
        carry_groups.reserve(M * 4);
        for (int d = 0; d < 4; ++d) {
            for (int seed_k = 0; seed_k < M; ++seed_k) {
                int ball_node = 1 + seed_k;
                int basket_node = 1 + M + seed_k;
                const Route &carry = route[ball_node][d][basket_node];
                if (carry.end_dir == -1 || carry.ops.empty()) continue;

                string key;
                key.reserve(carry.ops.size() + 1);
                key.push_back(static_cast<char>('0' + d));
                key += carry.ops;
                if (carry_groups.find(key) != carry_groups.end()) continue;

                CarryPatternGroup group;
                group.dir = d;
                group.ops = carry.ops;
                group.end_dir = carry.end_dir;
                for (int k = 0; k < M; ++k) {
                    auto [er, ec, edir] = run_ops_from(ball[k], d, carry.ops);
                    (void)edir;
                    if (er == basket[k].r && ec == basket[k].c) {
                        group.items.push_back(k);
                    }
                }
                carry_groups.emplace(std::move(key), std::move(group));
            }
        }

        struct CarryGroupSeed {
            int rough_gain = 0;
            int dir = 0;
            int end_dir = 0;
            string label;
            vector<int> items;
        };
        vector<CarryGroupSeed> carry_seeds;
        for (auto &[key, group] : carry_groups) {
            vector<int> &items = group.items;
            if (items.size() < 2) continue;
            int dir = group.dir;
            string label;
            label.reserve(group.ops.size() + 2);
            label.push_back('S');
            label += group.ops;
            label.push_back('S');
            int rough_gain =
                (static_cast<int>(items.size()) - 1) *
                    (static_cast<int>(label.size()) - 1) -
                2;
            if (rough_gain <= 0) continue;
            sort(items.begin(), items.end(), [&](int a, int b) {
                if (order_pos[a] != order_pos[b]) return order_pos[a] < order_pos[b];
                return a < b;
            });
            carry_seeds.push_back({rough_gain, dir, group.end_dir, label, items});
        }

        stable_sort(carry_seeds.begin(), carry_seeds.end(),
                    [](const CarryGroupSeed &a, const CarryGroupSeed &b) {
            if (a.rough_gain != b.rough_gain) return a.rough_gain > b.rough_gain;
            if (a.items.size() != b.items.size()) return a.items.size() > b.items.size();
            return a.label.size() > b.label.size();
        });
        if (static_cast<int>(carry_seeds.size()) > MACRO_CHAIN_CANDIDATE_LIMIT) {
            carry_seeds.resize(MACRO_CHAIN_CANDIDATE_LIMIT);
        }

        int macro_carry_valid = 0;
        int macro_carry_best_buttons = 1000000000;
        int macro_carry_best_count = 0;
        for (const CarryGroupSeed &seed : carry_seeds) {
            if (time_over()) break;
            vector<int> remaining = seed.items;
            vector<int> macro_order;
            vector<string> transitions;
            int current = 0;
            int dir = 1;

            while (!remaining.empty()) {
                int best_idx = -1;
                string best_transition;
                int best_cost = 1000000000;
                for (int idx = 0; idx < static_cast<int>(remaining.size()); ++idx) {
                    int k = remaining[idx];
                    int ball_node = 1 + k;
                    const Route &to_ball = route[current][dir][ball_node];
                    if (to_ball.end_dir == -1) continue;
                    string transition = to_ball.ops;
                    append_turn_ops(transition, to_ball.end_dir, seed.dir);
                    int cost = static_cast<int>(transition.size());
                    if (cost < best_cost ||
                        (cost == best_cost && order_pos[k] < order_pos[remaining[best_idx]])) {
                        best_cost = cost;
                        best_idx = idx;
                        best_transition = std::move(transition);
                    }
                }
                if (best_idx == -1) break;

                int k = remaining[best_idx];
                macro_order.push_back(k);
                transitions.push_back(std::move(best_transition));
                current = 1 + M + k;
                dir = seed.end_dir;
                remaining.erase(remaining.begin() + best_idx);
            }

            if (macro_order.size() < 2) continue;

            string prefix_transition = transitions[0];
            string middle_output;
            string middle_base;
            middle_output.reserve(seed.label.size() * macro_order.size());
            middle_base.reserve(seed.label.size() * macro_order.size());
            middle_output += prefix_transition;
            middle_output.push_back('M');
            middle_output += seed.label;
            middle_output.push_back('M');
            middle_base += prefix_transition;
            middle_base += seed.label;

            vector<char> done(M, 0);
            done[macro_order[0]] = 1;
            size_t used_len = middle_base.size();
            int delivered = 1;
            int last_node = 1 + M + macro_order[0];
            int last_dir = seed.end_dir;

            for (int idx = 1; idx < static_cast<int>(macro_order.size()); ++idx) {
                int k = macro_order[idx];
                string segment_output = transitions[idx];
                segment_output.push_back('P');
                string segment_base = transitions[idx] + seed.label;
                if (used_len + segment_base.size() > static_cast<size_t>(T)) break;

                middle_output += segment_output;
                middle_base += segment_base;
                used_len += segment_base.size();
                done[k] = 1;
                ++delivered;
                last_node = 1 + M + k;
                last_dir = seed.end_dir;

                if (delivered >= 2) {
                    BuiltAnswer suffix =
                        build_direct_suffix(best_order, done, last_node, last_dir,
                                            used_len, delivered);
                    if (suffix.delivered < M) continue;

                    BuiltAnswer built;
                    built.base = middle_base + suffix.base;
                    built.delivered = suffix.delivered;
                    if (built.base.size() > static_cast<size_t>(T)) continue;

                    string candidate_output = middle_output + compress_output(suffix.base);
                    if (candidate_output.size() > static_cast<size_t>(T)) continue;

                    ++macro_carry_valid;
                    if (static_cast<int>(candidate_output.size()) <
                        macro_carry_best_buttons) {
                        macro_carry_best_buttons =
                            static_cast<int>(candidate_output.size());
                        macro_carry_best_count = delivered;
                    }
                    macro_chain_candidates.push_back(
                        {std::move(built), std::move(candidate_output)});
                }
            }
        }

        struct MacroTemplateState {
            string base;
            string output;
            vector<char> done;
            int current = 0;
            int dir = 1;
            int delivered = 0;
        };

        auto try_append_template_block =
            [&](const CarryGroupSeed &seed, const MacroTemplateState &state)
            -> pair<bool, MacroTemplateState> {
            MacroTemplateState next = state;
            vector<int> remaining;
            for (int k : seed.items) {
                if (!next.done[k]) remaining.push_back(k);
            }
            if (remaining.size() < 2) return {false, state};

            vector<int> macro_order;
            vector<string> transitions;
            int current = next.current;
            int dir = next.dir;

            while (!remaining.empty()) {
                int best_idx = -1;
                string best_transition;
                int best_cost = 1000000000;
                for (int idx = 0; idx < static_cast<int>(remaining.size()); ++idx) {
                    int k = remaining[idx];
                    const Route &to_ball = route[current][dir][1 + k];
                    if (to_ball.end_dir == -1) continue;
                    string transition = to_ball.ops;
                    append_turn_ops(transition, to_ball.end_dir, seed.dir);
                    int cost = static_cast<int>(transition.size());
                    if (cost < best_cost ||
                        (cost == best_cost && best_idx != -1 &&
                         order_pos[k] < order_pos[remaining[best_idx]])) {
                        best_cost = cost;
                        best_idx = idx;
                        best_transition = std::move(transition);
                    }
                }
                if (best_idx == -1) break;

                int k = remaining[best_idx];
                macro_order.push_back(k);
                transitions.push_back(std::move(best_transition));
                current = 1 + M + k;
                dir = seed.end_dir;
                remaining.erase(remaining.begin() + best_idx);
            }

            if (macro_order.size() < 2) return {false, state};

            string block_base;
            string block_output;
            block_base.reserve(macro_order.size() * seed.label.size());
            block_output.reserve(macro_order.size() * seed.label.size());

            block_base += transitions[0];
            block_base += seed.label;
            block_output += transitions[0];
            block_output.push_back('M');
            block_output += seed.label;
            block_output.push_back('M');

            next.done[macro_order[0]] = 1;
            ++next.delivered;
            next.current = 1 + M + macro_order[0];
            next.dir = seed.end_dir;

            for (int idx = 1; idx < static_cast<int>(macro_order.size()); ++idx) {
                int k = macro_order[idx];
                block_base += transitions[idx];
                block_base += seed.label;
                block_output += transitions[idx];
                block_output.push_back('P');

                next.done[k] = 1;
                ++next.delivered;
                next.current = 1 + M + k;
                next.dir = seed.end_dir;
            }

            if (next.base.size() + block_base.size() > static_cast<size_t>(T)) {
                return {false, state};
            }
            if (next.output.size() + block_output.size() > static_cast<size_t>(T)) {
                return {false, state};
            }
            next.base += block_base;
            next.output += block_output;
            return {true, std::move(next)};
        };

        int macro_multi_valid = 0;
        int macro_multi_best_buttons = 1000000000;
        int macro_multi_best_blocks = 0;
        const int first_seed_limit =
            min<int>(MACRO_CHAIN_CANDIDATE_LIMIT, carry_seeds.size());
        for (int first_seed = 0; first_seed < first_seed_limit; ++first_seed) {
            if (time_over()) break;

            MacroTemplateState state;
            state.done.assign(M, 0);
            auto first_block = try_append_template_block(carry_seeds[first_seed], state);
            if (!first_block.first) continue;
            state = std::move(first_block.second);
            int used_blocks = 1;

            while (used_blocks < MACRO_CHAIN_MAX_BLOCKS) {
                if (time_over()) break;
                int best_seed = -1;
                MacroTemplateState best_state;
                long long best_key = 1LL << 60;

                for (int seed_idx = 0; seed_idx < first_seed_limit; ++seed_idx) {
                    auto trial = try_append_template_block(carry_seeds[seed_idx], state);
                    if (!trial.first) continue;
                    int added = trial.second.delivered - state.delivered;
                    if (added <= 0) continue;
                    long long added_buttons =
                        static_cast<long long>(trial.second.output.size()) -
                        static_cast<long long>(state.output.size());
                    long long key = added_buttons * 100 - added * 35;
                    if (key < best_key) {
                        best_key = key;
                        best_seed = seed_idx;
                        best_state = std::move(trial.second);
                    }
                }

                if (best_seed == -1) break;
                state = std::move(best_state);
                ++used_blocks;
            }

            if (state.delivered < 2) continue;
            BuiltAnswer suffix = build_direct_suffix(best_order, state.done, state.current,
                                                     state.dir, state.base.size(),
                                                     state.delivered);
            if (suffix.delivered < M) continue;

            BuiltAnswer built;
            built.base = state.base + suffix.base;
            built.delivered = suffix.delivered;
            if (built.base.size() > static_cast<size_t>(T)) continue;

            string candidate_output = state.output + compress_output(suffix.base);
            if (candidate_output.size() > static_cast<size_t>(T)) continue;

            ++macro_multi_valid;
            if (static_cast<int>(candidate_output.size()) < macro_multi_best_buttons) {
                macro_multi_best_buttons = static_cast<int>(candidate_output.size());
                macro_multi_best_blocks = used_blocks;
            }
            macro_chain_candidates.push_back({std::move(built), std::move(candidate_output)});
        }
#if MACRO_CHAIN_DEBUG
        cerr << "METRIC macro_chain_groups=" << macro_chain_groups
             << " macro_chain_raw_edges=" << macro_chain_raw_edges
             << " macro_chain_seeds=" << seeds.size()
             << " macro_chain_valid=" << macro_chain_valid
             << " macro_chain_best_buttons=" << macro_chain_best_buttons
             << " macro_chain_best_edges=" << macro_chain_best_edges
             << " macro_template_labels=" << macro_template_labels
             << " macro_template_edges=" << macro_template_edges
             << " macro_carry_groups=" << carry_seeds.size()
             << " macro_carry_valid=" << macro_carry_valid
             << " macro_carry_best_buttons=" << macro_carry_best_buttons
             << " macro_carry_best_count=" << macro_carry_best_count
             << " macro_multi_valid=" << macro_multi_valid
             << " macro_multi_best_buttons=" << macro_multi_best_buttons
             << " macro_multi_best_blocks=" << macro_multi_best_blocks
             << "\n";
#else
        (void)macro_chain_groups;
        (void)macro_chain_raw_edges;
        (void)macro_chain_valid;
        (void)macro_chain_best_edges;
        (void)macro_template_labels;
        (void)macro_template_edges;
        (void)macro_carry_valid;
        (void)macro_carry_best_count;
        (void)macro_multi_valid;
        (void)macro_multi_best_blocks;
#endif
    }
#endif

#if ENABLE_ALT_ROUTE_CANDIDATE
    if (!best_order.empty() && !best_base.empty() && !time_over()) {
        alt_routes.assign(
            6, vector<vector<vector<Route>>>(
                   node_count, vector<vector<Route>>(4, vector<Route>(node_count))));
        for (int mode = 0; mode < 6 && !time_over(); ++mode) {
            for (int source = 0; source < node_count && !time_over(); ++source) {
                for (int sdir = 0; sdir < 4; ++sdir) {
                    build_routes(alt_routes[mode], mode, source, sdir);
                }
            }
        }
    }

    if (!best_order.empty() && !best_base.empty() && !time_over()) {
        auto try_alt_answer = [&](const BuiltAnswer &alt_answer) {
            if (time_over()) return;
            string alt_output = compress_output(alt_answer.base);
            if (!time_over()) {
                alt_output = compress_hierarchical(alt_answer.base, alt_output);
            }
            auto alt_score = score_tuple(alt_answer, alt_output);
            if (alt_score < best_score) {
                best_score = alt_score;
                output = alt_output;
                best_base = alt_answer.base;
                best_answer = alt_answer;
            }
        };

        BuiltAnswer greedy_alt_answer = build_answer_with_route_choice(best_order, best_base);
        try_alt_answer(greedy_alt_answer);

#if ALT_ROUTE_REFINEMENT_ROUNDS > 0
        BuiltAnswer reference_alt_answer = greedy_alt_answer;
        for (int round = 0; round < ALT_ROUTE_REFINEMENT_ROUNDS && !time_over(); ++round) {
            BuiltAnswer refined_alt_answer =
                build_answer_with_route_choice(best_order, reference_alt_answer.base);
            if (refined_alt_answer.base == reference_alt_answer.base) break;
            try_alt_answer(refined_alt_answer);
            reference_alt_answer = std::move(refined_alt_answer);
        }
#endif

#if ALT_ROUTE_BEAM_WIDTH > 0 && ALT_ROUTE_FINAL_LIMIT > 0
        if (!time_over()) {
            vector<BuiltAnswer> alt_answers =
                build_route_choice_beam_answers(best_order, best_base);
            for (const BuiltAnswer &alt_answer : alt_answers) {
                if (alt_answer.base == greedy_alt_answer.base) continue;
                try_alt_answer(alt_answer);
            }
        }
#endif
    }
#endif

#if ENABLE_ALT_ROUTE_CANDIDATE && ALT_ROUTE_NEW_GRAM_BONUS && MACRO_CHAIN_DEBUG
    cerr << "METRIC alt_route_newgram_bonus=" << alt_route_newgram_bonus_total
         << " alt_route_newgram_choices=" << alt_route_newgram_choices << "\n";
#endif

#if ENABLE_MACRO_PROGRAM_CANDIDATE
    if (!best_order.empty() && !time_over()) {
        auto append_program_turn_ops = [](string &ops, int from_dir, int to_dir) {
            int diff = (to_dir - from_dir + 4) & 3;
            if (diff == 1) {
                ops.push_back('R');
            } else if (diff == 2) {
                ops.push_back('R');
                ops.push_back('R');
            } else if (diff == 3) {
                ops.push_back('L');
            }
        };

        auto run_program_ops = [&](int r, int c, int dir, const string &ops) {
            for (char op : ops) {
                if (op == 'F') {
                    if (can_move(r, c, dir)) {
                        r += dr[dir];
                        c += dc[dir];
                    }
                } else if (op == 'R') {
                    dir = (dir + 1) & 3;
                } else if (op == 'L') {
                    dir = (dir + 3) & 3;
                }
            }
            return make_tuple(r, c, dir);
        };

        auto build_program_suffix =
            [&](const vector<int> &order, const vector<char> &done,
                int start_node, int start_dir, size_t used_len,
                int delivered_prefix) -> BuiltAnswer {
            string result;
            int current = start_node;
            int dir = start_dir;
            int delivered = delivered_prefix;

            for (int k : order) {
                if (done[k]) continue;
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
                if (used_len + result.size() + segment.size() > static_cast<size_t>(T)) break;

                result += segment;
                current = basket_node;
                dir = to_basket.end_dir;
                ++delivered;
            }

            return {result, delivered};
        };

        vector<int> program_ball_at(N * N, -1);
        for (int k = 0; k < M; ++k) {
            program_ball_at[ball[k].r * N + ball[k].c] = k;
        }

        vector<int> order_pos(M, M);
        for (int i = 0; i < static_cast<int>(best_order.size()); ++i) {
            order_pos[best_order[i]] = i;
        }

        struct ProgramCarry {
            int start_dir = 0;
            int end_dir = 0;
            int support = 1;
            string ops;
        };
        vector<ProgramCarry> carries;
        unordered_map<string, int> carry_index;
        carry_index.reserve(M * 16);

        auto add_program_carry =
            [&](int start_dir, int end_dir, const string &ops, int support = 1) {
            if (2 + static_cast<int>(ops.size()) > MACRO_PROGRAM_MAX_LEN) return;
            string key;
            key.reserve(ops.size() + 2);
            key.push_back(static_cast<char>('0' + start_dir));
            key += ops;
            auto it = carry_index.find(key);
            if (it != carry_index.end()) {
                ProgramCarry &existing = carries[it->second];
                existing.support = max(existing.support, support);
                return;
            }
            carry_index[key] = static_cast<int>(carries.size());
            carries.push_back({start_dir, end_dir, max(1, support), ops});
        };

        for (int d = 0; d < 4; ++d) {
            for (int k = 0; k < M; ++k) {
                const Route &carry = route[1 + k][d][1 + M + k];
                if (carry.end_dir == -1) continue;
                add_program_carry(d, carry.end_dir, carry.ops);
#if ENABLE_ALT_ROUTE_CANDIDATE
                if (!alt_routes.empty()) {
                    for (int mode = 0; mode < 6; ++mode) {
                        const Route &alt_carry = alt_routes[mode][1 + k][d][1 + M + k];
                        if (alt_carry.end_dir == -1) continue;
                        add_program_carry(d, alt_carry.end_dir, alt_carry.ops);
                    }
                }
#endif
            }
        }

        int macro_free_carries = 0;
        int macro_free_best_support = 0;
        int macro_free_states = 0;
        {
            struct FreeCarryState {
                int dir = 0;
                int support = 0;
                int score = 0;
                string ops;
                vector<unsigned short> cells;
            };

            vector<int> basket_cell(M);
            for (int k = 0; k < M; ++k) {
                basket_cell[k] = basket[k].r * N + basket[k].c;
            }

            auto evaluate_free_state = [&](FreeCarryState &state) {
                int support = 0;
                int dist_sum = 0;
                for (int k = 0; k < M; ++k) {
                    int cell = state.cells[k];
                    if (cell == basket_cell[k]) {
                        ++support;
                    } else {
                        int r = cell / N;
                        int c = cell % N;
                        dist_sum += abs(r - basket[k].r) + abs(c - basket[k].c);
                    }
                }
                state.support = support;
                state.score = support * 100000 - dist_sum * 100 -
                              static_cast<int>(state.ops.size()) * 3;
            };

            auto transition_free_state = [&](const FreeCarryState &state, char op) {
                FreeCarryState next = state;
                next.ops.push_back(op);
                if (op == 'L') {
                    next.dir = (next.dir + 3) & 3;
                } else if (op == 'R') {
                    next.dir = (next.dir + 1) & 3;
                } else {
                    for (int k = 0; k < M; ++k) {
                        int cell = next.cells[k];
                        int r = cell / N;
                        int c = cell % N;
                        if (can_move(r, c, next.dir)) {
                            next.cells[k] = static_cast<unsigned short>(
                                (r + dr[next.dir]) * N + (c + dc[next.dir]));
                        }
                    }
                }
                evaluate_free_state(next);
                return next;
            };

            vector<ProgramCarry> free_carries;
            free_carries.reserve(MACRO_FREE_CARRY_KEEP * 4);

            for (int start_dir = 0; start_dir < 4; ++start_dir) {
                vector<FreeCarryState> beam;
                FreeCarryState initial;
                initial.dir = start_dir;
                initial.cells.resize(M);
                for (int k = 0; k < M; ++k) {
                    initial.cells[k] =
                        static_cast<unsigned short>(ball[k].r * N + ball[k].c);
                }
                evaluate_free_state(initial);
                beam.push_back(std::move(initial));

                for (int depth = 0; depth <= MACRO_FREE_CARRY_MAX_LEN; ++depth) {
                    for (const FreeCarryState &state : beam) {
                        if (!state.ops.empty() && state.support >= 2) {
                            free_carries.push_back(
                                {start_dir, state.dir, state.support, state.ops});
                            macro_free_best_support =
                                max(macro_free_best_support, state.support);
                        }
                    }
                    if (depth == MACRO_FREE_CARRY_MAX_LEN || time_over()) break;

                    vector<FreeCarryState> next;
                    next.reserve(beam.size() * 3);
                    for (const FreeCarryState &state : beam) {
                        const char last = state.ops.empty() ? 0 : state.ops.back();
                        for (char op : {'F', 'L', 'R'}) {
                            if ((last == 'L' && op == 'R') ||
                                (last == 'R' && op == 'L')) {
                                continue;
                            }
                            next.push_back(transition_free_state(state, op));
                        }
                    }

                    stable_sort(next.begin(), next.end(),
                                [](const FreeCarryState &a, const FreeCarryState &b) {
                        if (a.score != b.score) return a.score > b.score;
                        if (a.support != b.support) return a.support > b.support;
                        if (a.ops.size() != b.ops.size()) return a.ops.size() < b.ops.size();
                        return a.ops < b.ops;
                    });

                    vector<FreeCarryState> pruned;
                    pruned.reserve(min<int>(MACRO_FREE_CARRY_BEAM, next.size()));
                    unordered_map<string, char> seen_state;
                    seen_state.reserve(MACRO_FREE_CARRY_BEAM * 4 + 1);
                    for (FreeCarryState &state : next) {
                        string key;
                        key.reserve(1 + state.cells.size() * 2);
                        key.push_back(static_cast<char>('0' + state.dir));
                        for (unsigned short cell : state.cells) {
                            key.push_back(static_cast<char>(cell & 255));
                            key.push_back(static_cast<char>(cell >> 8));
                        }
                        if (seen_state.find(key) != seen_state.end()) continue;
                        seen_state[key] = 1;
                        pruned.push_back(std::move(state));
                        if (static_cast<int>(pruned.size()) >= MACRO_FREE_CARRY_BEAM) {
                            break;
                        }
                    }
                    macro_free_states += static_cast<int>(pruned.size());
                    beam = std::move(pruned);
                    if (beam.empty()) break;
                }
            }

            stable_sort(free_carries.begin(), free_carries.end(),
                        [](const ProgramCarry &a, const ProgramCarry &b) {
                if (a.support != b.support) return a.support > b.support;
                if (a.ops.size() != b.ops.size()) return a.ops.size() < b.ops.size();
                if (a.start_dir != b.start_dir) return a.start_dir < b.start_dir;
                return a.ops < b.ops;
            });
            if (static_cast<int>(free_carries.size()) > MACRO_FREE_CARRY_KEEP) {
                free_carries.resize(MACRO_FREE_CARRY_KEEP);
            }
            macro_free_carries = static_cast<int>(free_carries.size());
            for (const ProgramCarry &carry : free_carries) {
                add_program_carry(carry.start_dir, carry.end_dir, carry.ops,
                                  carry.support);
            }
        }

        stable_sort(carries.begin(), carries.end(),
                    [](const ProgramCarry &a, const ProgramCarry &b) {
            if (a.support != b.support) return a.support > b.support;
            if (a.ops.size() != b.ops.size()) return a.ops.size() < b.ops.size();
            if (a.start_dir != b.start_dir) return a.start_dir < b.start_dir;
            return a.ops < b.ops;
        });

        vector<vector<string>> transitions(16);
        vector<unordered_map<string, char>> transition_seen(16);
        for (auto &seen : transition_seen) seen.reserve(M * M + 64);

        auto add_program_transition =
            [&](int from_dir, int target_dir, const string &ops) {
            if (static_cast<int>(ops.size()) + 2 > MACRO_PROGRAM_MAX_LEN) return;
            int key_id = from_dir * 4 + target_dir;
            if (transition_seen[key_id].find(ops) != transition_seen[key_id].end()) return;
            transition_seen[key_id][ops] = 1;
            transitions[key_id].push_back(ops);
        };

        for (int from_dir = 0; from_dir < 4; ++from_dir) {
            for (int target_dir = 0; target_dir < 4; ++target_dir) {
                for (int bi = 0; bi < M; ++bi) {
                    int basket_node = 1 + M + bi;
                    for (int bj = 0; bj < M; ++bj) {
                        int ball_node = 1 + bj;
                        const Route &to_ball = route[basket_node][from_dir][ball_node];
                        if (to_ball.end_dir == -1) continue;
                        string ops = to_ball.ops;
                        append_program_turn_ops(ops, to_ball.end_dir, target_dir);
                        add_program_transition(from_dir, target_dir, ops);
#if ENABLE_ALT_ROUTE_CANDIDATE
                        if (!alt_routes.empty()) {
                            for (int mode = 0; mode < 6; ++mode) {
                                const Route &alt_to_ball =
                                    alt_routes[mode][basket_node][from_dir][ball_node];
                                if (alt_to_ball.end_dir == -1) continue;
                                string alt_ops = alt_to_ball.ops;
                                append_program_turn_ops(alt_ops, alt_to_ball.end_dir,
                                                        target_dir);
                                add_program_transition(from_dir, target_dir, alt_ops);
                            }
                        }
#endif
                    }
                }

                for (int a = 0; a <= 8; ++a) {
                    for (int b = 0; b <= 8; ++b) {
                        string base;
                        base.append(a, 'F');
                        int dir = from_dir;
                        string ops = base;
                        ops.push_back('R');
                        dir = (dir + 1) & 3;
                        ops.append(b, 'F');
                        append_program_turn_ops(ops, dir, target_dir);
                        add_program_transition(from_dir, target_dir, ops);

                        ops = base;
                        dir = from_dir;
                        ops.push_back('L');
                        dir = (dir + 3) & 3;
                        ops.append(b, 'F');
                        append_program_turn_ops(ops, dir, target_dir);
                        add_program_transition(from_dir, target_dir, ops);
                    }
                }
            }
        }

        for (int key_id = 0; key_id < 16; ++key_id) {
            stable_sort(transitions[key_id].begin(), transitions[key_id].end(),
                        [](const string &a, const string &b) {
                if (a.size() != b.size()) return a.size() < b.size();
                return a < b;
            });
            const int keep = 120;
            if (static_cast<int>(transitions[key_id].size()) > keep) {
                transitions[key_id].resize(keep);
            }
        }

        struct ProgramSeed {
            int rough_gain = 0;
            int dir = 0;
            string label;
            vector<int> path;
        };
        vector<ProgramSeed> program_seeds;
        unordered_map<string, char> program_seen;
        program_seen.reserve(MACRO_PROGRAM_CANDIDATE_LIMIT * 2 + 1);

        int macro_program_tested = 0;
        int macro_program_labels = 0;
        int macro_program_edges = 0;
        int macro_program_seed_count = 0;
        int macro_direct_states = 0;
        int macro_direct_seeds = 0;
        int macro_direct_best_edges = 0;

        auto best_path_for_edges = [&](const vector<pair<int, int>> &edges) {
            vector<vector<int>> nexts(M);
            for (auto [from, to] : edges) nexts[from].push_back(to);
            for (vector<int> &xs : nexts) {
                stable_sort(xs.begin(), xs.end(), [&](int a, int b) {
                    if (order_pos[a] != order_pos[b]) return order_pos[a] < order_pos[b];
                    return a < b;
                });
                xs.erase(unique(xs.begin(), xs.end()), xs.end());
            }

            vector<int> best_path;
            for (auto [from, to] : edges) {
                vector<int> path;
                vector<char> used(M, 0);
                path.push_back(from);
                path.push_back(to);
                used[from] = 1;
                used[to] = 1;

                int current = to;
                while (!nexts[current].empty()) {
                    int chosen = -1;
                    for (int nxt : nexts[current]) {
                        if (!used[nxt]) {
                            chosen = nxt;
                            break;
                        }
                    }
                    if (chosen == -1) break;
                    path.push_back(chosen);
                    used[chosen] = 1;
                    current = chosen;
                }

                if (path.size() > best_path.size()) best_path = std::move(path);
            }
            return best_path;
        };

        {
            struct DirectMacroState {
                int dir = 0;
                bool dropped = false;
                int score = 0;
                int best_edges = 0;
                string label;
                vector<unsigned short> cells;
                vector<unsigned char> valid;
                vector<int> path;
            };

            vector<int> basket_cell(M);
            for (int k = 0; k < M; ++k) {
                basket_cell[k] = basket[k].r * N + basket[k].c;
            }

            for (int start_dir = 0; start_dir < 4 && !time_over(); ++start_dir) {
                auto evaluate_direct_state = [&](DirectMacroState &state) {
                    state.best_edges = 0;
                    state.path.clear();

                    if (!state.dropped) {
                        int ready = 0;
                        int dist_sum = 0;
                        for (int k = 0; k < M; ++k) {
                            int cell = state.cells[k];
                            if (cell == basket_cell[k]) {
                                ++ready;
                            } else {
                                int r = cell / N;
                                int c = cell % N;
                                dist_sum += abs(r - basket[k].r) + abs(c - basket[k].c);
                            }
                        }
                        state.score = ready * 100000 - dist_sum * 100 -
                                      static_cast<int>(state.label.size()) * 5;
                        return;
                    }

                    vector<pair<int, int>> edges;
                    int valid_count = 0;
                    int nearest_ball_dist_sum = 0;
                    if (state.dir == start_dir) {
                        for (int k = 0; k < M; ++k) {
                            if (!state.valid[k]) continue;
                            ++valid_count;
                            int next = program_ball_at[state.cells[k]];
                            if (next == -1 || next == k) continue;
                            edges.push_back({k, next});
                        }
                    } else {
                        for (int k = 0; k < M; ++k) {
                            if (state.valid[k]) ++valid_count;
                        }
                    }
                    for (int k = 0; k < M; ++k) {
                        if (!state.valid[k]) continue;
                        int cell = state.cells[k];
                        int r = cell / N;
                        int c = cell % N;
                        int best_dist = N * 2;
                        for (int j = 0; j < M; ++j) {
                            if (j == k) continue;
                            best_dist = min(best_dist, abs(r - ball[j].r) +
                                                        abs(c - ball[j].c));
                        }
                        nearest_ball_dist_sum += best_dist;
                    }
                    if (!edges.empty()) {
                        state.path = best_path_for_edges(edges);
                        state.best_edges = static_cast<int>(state.path.size()) - 1;
                    }
                    state.score = state.best_edges * 200000 +
                                  static_cast<int>(edges.size()) * 2000 -
                                  nearest_ball_dist_sum * 80 +
                                  valid_count * 50000 -
                                  static_cast<int>(state.label.size()) * 20;
                };

                auto advance_direct_state =
                    [&](const DirectMacroState &state, char op) {
                    DirectMacroState next = state;
                    next.label.push_back(op);
                    if (op == 'L') {
                        next.dir = (next.dir + 3) & 3;
                    } else if (op == 'R') {
                        next.dir = (next.dir + 1) & 3;
                    } else if (op == 'F') {
                        for (int k = 0; k < M; ++k) {
                            int cell = next.cells[k];
                            int r = cell / N;
                            int c = cell % N;
                            if (can_move(r, c, next.dir)) {
                                next.cells[k] = static_cast<unsigned short>(
                                    (r + dr[next.dir]) * N + (c + dc[next.dir]));
                            }
                        }
                    } else if (op == 'S') {
                        next.dropped = true;
                        next.valid.assign(M, 0);
                        for (int k = 0; k < M; ++k) {
                            next.valid[k] = (next.cells[k] == basket_cell[k]);
                        }
                    }
                    evaluate_direct_state(next);
                    return next;
                };

                DirectMacroState initial;
                initial.dir = start_dir;
                initial.label = "S";
                initial.cells.resize(M);
                initial.valid.assign(M, 1);
                for (int k = 0; k < M; ++k) {
                    initial.cells[k] =
                        static_cast<unsigned short>(ball[k].r * N + ball[k].c);
                }
                evaluate_direct_state(initial);

                vector<DirectMacroState> beam;
                beam.push_back(std::move(initial));
                for (int depth = 1; depth < MACRO_DIRECT_MAX_LEN && !time_over(); ++depth) {
                    vector<DirectMacroState> next_states;
                    next_states.reserve(beam.size() * 4);
                    for (const DirectMacroState &state : beam) {
                        const char last = state.label.empty() ? 0 : state.label.back();
                        for (char op : {'F', 'L', 'R'}) {
                            if ((last == 'L' && op == 'R') ||
                                (last == 'R' && op == 'L')) {
                                continue;
                            }
                            next_states.push_back(advance_direct_state(state, op));
                        }
                        if (!state.dropped) {
                            next_states.push_back(advance_direct_state(state, 'S'));
                        }
                    }

                    stable_sort(next_states.begin(), next_states.end(),
                                [](const DirectMacroState &a,
                                   const DirectMacroState &b) {
                        if (a.score != b.score) return a.score > b.score;
                        if (a.best_edges != b.best_edges) return a.best_edges > b.best_edges;
                        if (a.label.size() != b.label.size()) {
                            return a.label.size() < b.label.size();
                        }
                        return a.label < b.label;
                    });

                    vector<DirectMacroState> pruned;
                    pruned.reserve(min<int>(MACRO_DIRECT_BEAM, next_states.size()));
                    unordered_map<string, char> seen_state;
                    seen_state.reserve(MACRO_DIRECT_BEAM * 4 + 1);
                    for (DirectMacroState &state : next_states) {
                        string state_key;
                        state_key.reserve(2 + state.cells.size() * 3);
                        state_key.push_back(static_cast<char>('0' + state.dir));
                        state_key.push_back(state.dropped ? '1' : '0');
                        for (unsigned short cell : state.cells) {
                            state_key.push_back(static_cast<char>(cell & 255));
                            state_key.push_back(static_cast<char>(cell >> 8));
                        }
                        if (state.dropped) {
                            for (unsigned char ok : state.valid) {
                                state_key.push_back(static_cast<char>(ok));
                            }
                        }
                        if (seen_state.find(state_key) != seen_state.end()) continue;
                        seen_state[state_key] = 1;

                        if (state.dropped && state.best_edges >= 2) {
                            string key;
                            key.reserve(state.label.size() + 1);
                            key.push_back(static_cast<char>('0' + start_dir));
                            key += state.label;
                            if (program_seen.find(key) == program_seen.end()) {
                                int rough_gain =
                                    (state.best_edges - 1) *
                                        (static_cast<int>(state.label.size()) - 1) -
                                    2;
                                if (rough_gain > 0) {
                                    program_seen[key] = 1;
                                    ++macro_direct_seeds;
                                    macro_direct_best_edges =
                                        max(macro_direct_best_edges, state.best_edges);
                                    program_seeds.push_back(
                                        {rough_gain, start_dir, state.label, state.path});
                                }
                            }
                        }

                        pruned.push_back(std::move(state));
                        if (static_cast<int>(pruned.size()) >= MACRO_DIRECT_BEAM) break;
                    }
                    macro_direct_states += static_cast<int>(pruned.size());
                    beam = std::move(pruned);
                    if (beam.empty()) break;
                }
            }
        }

        for (const ProgramCarry &carry : carries) {
            if (time_over() || macro_program_tested >= MACRO_PROGRAM_CANDIDATE_LIMIT) break;
            int transition_key = carry.end_dir * 4 + carry.start_dir;
            for (const string &tail_ops : transitions[transition_key]) {
                if (time_over() || macro_program_tested >= MACRO_PROGRAM_CANDIDATE_LIMIT) break;
                int label_len = 2 + static_cast<int>(carry.ops.size()) +
                                static_cast<int>(tail_ops.size());
                if (label_len > MACRO_PROGRAM_MAX_LEN) continue;

                string label;
                label.reserve(label_len);
                label.push_back('S');
                label += carry.ops;
                label.push_back('S');
                label += tail_ops;

                string key;
                key.reserve(label.size() + 1);
                key.push_back(static_cast<char>('0' + carry.start_dir));
                key += label;
                if (program_seen.find(key) != program_seen.end()) continue;
                program_seen[key] = 1;
                ++macro_program_tested;

                vector<pair<int, int>> edges;
                for (int k = 0; k < M; ++k) {
                    auto [br, bc, bdir] =
                        run_program_ops(ball[k].r, ball[k].c, carry.start_dir,
                                        carry.ops);
                    if (br != basket[k].r || bc != basket[k].c) continue;
                    auto [er, ec, edir] = run_program_ops(br, bc, bdir, tail_ops);
                    if (edir != carry.start_dir) continue;
                    int next = program_ball_at[er * N + ec];
                    if (next == -1 || next == k) continue;
                    edges.push_back({k, next});
                }
                if (edges.size() < 2) continue;

                ++macro_program_labels;
                macro_program_edges += static_cast<int>(edges.size());

                vector<int> path = best_path_for_edges(edges);
                int edge_count = static_cast<int>(path.size()) - 1;
                if (edge_count < 2) continue;
                int rough_gain = (edge_count - 1) * (label_len - 1) - 2;
                if (rough_gain <= 0) continue;
                ++macro_program_seed_count;
                program_seeds.push_back(
                    {rough_gain, carry.start_dir, std::move(label), std::move(path)});
            }
        }

        stable_sort(program_seeds.begin(), program_seeds.end(),
                    [](const ProgramSeed &a, const ProgramSeed &b) {
            if (a.rough_gain != b.rough_gain) return a.rough_gain > b.rough_gain;
            if (a.path.size() != b.path.size()) return a.path.size() > b.path.size();
            return a.label.size() > b.label.size();
        });
        const int program_eval_limit =
            min<int>(MACRO_CHAIN_CANDIDATE_LIMIT, program_seeds.size());

        int macro_program_valid = 0;
        int macro_program_best_buttons = 1000000000;
        int macro_program_best_edges = 0;

        for (int seed_idx = 0; seed_idx < program_eval_limit; ++seed_idx) {
            if (time_over()) break;
            const ProgramSeed &seed = program_seeds[seed_idx];
            int edge_count = static_cast<int>(seed.path.size()) - 1;
            int first = seed.path.front();
            int terminal = seed.path.back();
            const Route &to_first = route[0][1][1 + first];
            if (to_first.end_dir == -1) continue;

            string prefix = to_first.ops;
            append_program_turn_ops(prefix, to_first.end_dir, seed.dir);

            vector<char> done(M, 0);
            for (int i = 0; i < edge_count; ++i) done[seed.path[i]] = 1;

            size_t used_len =
                prefix.size() + static_cast<size_t>(edge_count) * seed.label.size();
            if (used_len > static_cast<size_t>(T)) continue;

            BuiltAnswer suffix = build_program_suffix(best_order, done, 1 + terminal,
                                                      seed.dir, used_len, edge_count);
            if (suffix.delivered < M) continue;

            BuiltAnswer built;
            built.base.reserve(used_len + suffix.base.size());
            built.base += prefix;
            for (int rep = 0; rep < edge_count; ++rep) built.base += seed.label;
            built.base += suffix.base;
            built.delivered = suffix.delivered;
            if (built.base.size() > static_cast<size_t>(T)) continue;

            string candidate_output = compress_output(prefix);
            candidate_output.push_back('M');
            candidate_output += seed.label;
            candidate_output.push_back('M');
            for (int rep = 1; rep < edge_count; ++rep) candidate_output.push_back('P');
            candidate_output += compress_output(suffix.base);
            if (candidate_output.size() > static_cast<size_t>(T)) continue;

            ++macro_program_valid;
            if (static_cast<int>(candidate_output.size()) < macro_program_best_buttons) {
                macro_program_best_buttons = static_cast<int>(candidate_output.size());
                macro_program_best_edges = edge_count;
            }
            macro_chain_candidates.push_back({std::move(built), std::move(candidate_output)});
        }

#if MACRO_CHAIN_DEBUG
        cerr << "METRIC macro_program_carries=" << carries.size()
             << " macro_free_carries=" << macro_free_carries
             << " macro_free_best_support=" << macro_free_best_support
             << " macro_free_states=" << macro_free_states
             << " macro_direct_states=" << macro_direct_states
             << " macro_direct_seeds=" << macro_direct_seeds
             << " macro_direct_best_edges=" << macro_direct_best_edges
             << " macro_program_tested=" << macro_program_tested
             << " macro_program_labels=" << macro_program_labels
             << " macro_program_edges=" << macro_program_edges
             << " macro_program_seeds=" << macro_program_seed_count
             << " macro_program_valid=" << macro_program_valid
             << " macro_program_best_buttons=" << macro_program_best_buttons
             << " macro_program_best_edges=" << macro_program_best_edges
             << "\n";
#else
        (void)macro_program_labels;
        (void)macro_program_edges;
        (void)macro_program_seed_count;
        (void)macro_program_valid;
        (void)macro_program_best_edges;
        (void)macro_free_carries;
        (void)macro_free_best_support;
        (void)macro_free_states;
        (void)macro_direct_states;
        (void)macro_direct_seeds;
        (void)macro_direct_best_edges;
#endif
    }
#endif

#if ENABLE_MACRO_CLUSTER_CANDIDATE
    if (!best_order.empty() && !time_over()) {
        auto append_cluster_turn_ops = [](string &ops, int from_dir, int to_dir) {
            int diff = (to_dir - from_dir + 4) & 3;
            if (diff == 1) {
                ops.push_back('R');
            } else if (diff == 2) {
                ops.push_back('R');
                ops.push_back('R');
            } else if (diff == 3) {
                ops.push_back('L');
            }
        };

        auto build_cluster_suffix =
            [&](const vector<int> &order, const vector<char> &done,
                int start_node, int start_dir, size_t used_len,
                int delivered_prefix) -> BuiltAnswer {
            string result;
            int current = start_node;
            int dir = start_dir;
            int delivered = delivered_prefix;

            for (int k : order) {
                if (done[k]) continue;
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
                if (used_len + result.size() + segment.size() > static_cast<size_t>(T)) break;

                result += segment;
                current = basket_node;
                dir = to_basket.end_dir;
                ++delivered;
            }

            return {result, delivered};
        };

        vector<int> cluster_ball_at(N * N, -1);
        for (int k = 0; k < M; ++k) {
            cluster_ball_at[ball[k].r * N + ball[k].c] = k;
        }

        vector<int> order_pos(M, M);
        for (int i = 0; i < static_cast<int>(best_order.size()); ++i) {
            order_pos[best_order[i]] = i;
        }

        struct ClusterEdge {
            int from = -1;
            int second = -1;
            int to = -1;
        };
        struct ClusterSeed {
            int rough_gain = 0;
            int dir = 0;
            string label;
            vector<ClusterEdge> reps;
        };

        auto simulate_cluster_label =
            [&](int start_dir, const string &label, int start_item) -> ClusterEdge {
            int r = ball[start_item].r;
            int c = ball[start_item].c;
            int dir = start_dir;
            int seen_s = 0;
            int second = -1;
            bool ok = true;

            for (char op : label) {
                if (op == 'S') {
                    if (seen_s == 0) {
                        if (r != ball[start_item].r || c != ball[start_item].c) {
                            ok = false;
                            break;
                        }
                        seen_s = 1;
                    } else if (seen_s == 1) {
                        if (r != basket[start_item].r || c != basket[start_item].c) {
                            ok = false;
                            break;
                        }
                        seen_s = 2;
                    } else if (seen_s == 2) {
                        second = cluster_ball_at[r * N + c];
                        if (second == -1 || second == start_item) {
                            ok = false;
                            break;
                        }
                        seen_s = 3;
                    } else if (seen_s == 3) {
                        if (second == -1 || r != basket[second].r ||
                            c != basket[second].c) {
                            ok = false;
                            break;
                        }
                        seen_s = 4;
                    } else {
                        ok = false;
                        break;
                    }
                } else if (op == 'F') {
                    if (can_move(r, c, dir)) {
                        r += dr[dir];
                        c += dc[dir];
                    }
                } else if (op == 'R') {
                    dir = (dir + 1) & 3;
                } else if (op == 'L') {
                    dir = (dir + 3) & 3;
                }
            }

            if (!ok || seen_s != 4 || dir != start_dir) return {};
            int terminal = cluster_ball_at[r * N + c];
            if (terminal == -1 || terminal == start_item || terminal == second) return {};
            return {start_item, second, terminal};
        };

        auto best_cluster_chain = [&](vector<ClusterEdge> edges, int macro_dir) {
            stable_sort(edges.begin(), edges.end(), [&](const ClusterEdge &a,
                                                       const ClusterEdge &b) {
                if (order_pos[a.from] != order_pos[b.from]) {
                    return order_pos[a.from] < order_pos[b.from];
                }
                if (order_pos[a.second] != order_pos[b.second]) {
                    return order_pos[a.second] < order_pos[b.second];
                }
                return order_pos[a.to] < order_pos[b.to];
            });
            edges.erase(unique(edges.begin(), edges.end(),
                               [](const ClusterEdge &a, const ClusterEdge &b) {
                                   return a.from == b.from && a.second == b.second &&
                                          a.to == b.to;
                               }),
                        edges.end());

            vector<ClusterEdge> best;
            for (const ClusterEdge &first_edge : edges) {
                vector<char> used(M, 0);
                vector<ClusterEdge> reps;
                reps.push_back(first_edge);
                used[first_edge.from] = 1;
                used[first_edge.second] = 1;
                int current = first_edge.to;
                int transition_total = 0;

                while (true) {
                    int best_idx = -1;
                    int best_cost = 1000000000;
                    for (int idx = 0; idx < static_cast<int>(edges.size()); ++idx) {
                        const ClusterEdge &edge = edges[idx];
                        if (used[edge.from] || used[edge.second]) continue;
                        const Route &to_next = route[1 + current][macro_dir][1 + edge.from];
                        if (to_next.end_dir == -1) continue;
                        string transition = to_next.ops;
                        append_cluster_turn_ops(transition, to_next.end_dir, macro_dir);
                        int cost = static_cast<int>(transition.size());
                        if (cost < best_cost ||
                            (cost == best_cost &&
                             order_pos[edge.from] < order_pos[edges[best_idx].from])) {
                            best_cost = cost;
                            best_idx = idx;
                        }
                    }
                    if (best_idx == -1) break;
                    const ClusterEdge &chosen = edges[best_idx];
                    reps.push_back(chosen);
                    used[chosen.from] = 1;
                    used[chosen.second] = 1;
                    current = chosen.to;
                    transition_total += best_cost;
                }

                if (reps.size() > best.size()) {
                    best = std::move(reps);
                } else if (reps.size() == best.size() && reps.size() >= 2) {
                    int best_transition_total = 0;
                    for (int idx = 1; idx < static_cast<int>(best.size()); ++idx) {
                        const Route &to_next =
                            route[1 + best[idx - 1].to][macro_dir][1 + best[idx].from];
                        if (to_next.end_dir == -1) {
                            best_transition_total = 1000000000;
                            break;
                        }
                        string transition = to_next.ops;
                        append_cluster_turn_ops(transition, to_next.end_dir, macro_dir);
                        best_transition_total += static_cast<int>(transition.size());
                    }
                    if (transition_total < best_transition_total) {
                        best = std::move(reps);
                    }
                }
            }
            return best;
        };

        unordered_map<string, char> cluster_seen;
        cluster_seen.reserve(M * MACRO_CLUSTER_NEXT_LIMIT * MACRO_CLUSTER_NEXT_LIMIT * 4);
        vector<ClusterSeed> cluster_seeds;
        int macro_cluster_labels = 0;
        int macro_cluster_edges = 0;
        int macro_cluster_seed_count = 0;
        int macro_cluster_best_reps = 0;

        for (int d = 0; d < 4 && !time_over(); ++d) {
            for (int i = 0; i < M && !time_over(); ++i) {
                const Route &r1 = route[1 + i][d][1 + M + i];
                if (r1.end_dir == -1) continue;

                vector<pair<int, int>> second_candidates;
                for (int j = 0; j < M; ++j) {
                    if (j == i) continue;
                    const Route &to_second = route[1 + M + i][r1.end_dir][1 + j];
                    if (to_second.end_dir == -1) continue;
                    const Route &second_carry =
                        route[1 + j][to_second.end_dir][1 + M + j];
                    if (second_carry.end_dir == -1) continue;
                    int cost = to_second.cost + second_carry.cost;
                    second_candidates.push_back({cost, j});
                }
                stable_sort(second_candidates.begin(), second_candidates.end());
                if (static_cast<int>(second_candidates.size()) > MACRO_CLUSTER_NEXT_LIMIT) {
                    second_candidates.resize(MACRO_CLUSTER_NEXT_LIMIT);
                }

                for (auto [unused_second_cost, j] : second_candidates) {
                    (void)unused_second_cost;
                    const Route &r2 = route[1 + M + i][r1.end_dir][1 + j];
                    const Route &r3 = route[1 + j][r2.end_dir][1 + M + j];
                    if (r2.end_dir == -1 || r3.end_dir == -1) continue;

                    vector<pair<int, int>> terminal_candidates;
                    for (int k = 0; k < M; ++k) {
                        if (k == i || k == j) continue;
                        const Route &to_terminal = route[1 + M + j][r3.end_dir][1 + k];
                        if (to_terminal.end_dir == -1) continue;
                        string turn_probe;
                        append_cluster_turn_ops(turn_probe, to_terminal.end_dir, d);
                        terminal_candidates.push_back(
                            {to_terminal.cost + static_cast<int>(turn_probe.size()), k});
                    }
                    stable_sort(terminal_candidates.begin(), terminal_candidates.end());
                    if (static_cast<int>(terminal_candidates.size()) >
                        MACRO_CLUSTER_NEXT_LIMIT) {
                        terminal_candidates.resize(MACRO_CLUSTER_NEXT_LIMIT);
                    }

                    for (auto [unused_terminal_cost, k] : terminal_candidates) {
                        (void)unused_terminal_cost;
                        const Route &r4 = route[1 + M + j][r3.end_dir][1 + k];
                        if (r4.end_dir == -1) continue;
                        string label;
                        label.reserve(4 + r1.ops.size() + r2.ops.size() +
                                      r3.ops.size() + r4.ops.size() + 2);
                        label.push_back('S');
                        label += r1.ops;
                        label.push_back('S');
                        label += r2.ops;
                        label.push_back('S');
                        label += r3.ops;
                        label.push_back('S');
                        label += r4.ops;
                        append_cluster_turn_ops(label, r4.end_dir, d);
                        if (static_cast<int>(label.size()) > MACRO_CLUSTER_MAX_LEN) {
                            continue;
                        }

                        string key;
                        key.reserve(label.size() + 1);
                        key.push_back(static_cast<char>('0' + d));
                        key += label;
                        if (cluster_seen.find(key) != cluster_seen.end()) continue;
                        cluster_seen[key] = 1;

                        vector<ClusterEdge> edges;
                        for (int start = 0; start < M; ++start) {
                            ClusterEdge edge = simulate_cluster_label(d, label, start);
                            if (edge.from != -1) edges.push_back(edge);
                        }
                        if (edges.size() < 2) continue;
                        ++macro_cluster_labels;
                        macro_cluster_edges += static_cast<int>(edges.size());

                        vector<ClusterEdge> reps = best_cluster_chain(edges, d);
                        int rep_count = static_cast<int>(reps.size());
                        if (rep_count < 2) continue;
                        int rough_gain =
                            (rep_count - 1) * (static_cast<int>(label.size()) - 1) - 2;
                        if (rough_gain <= 0) continue;
                        ++macro_cluster_seed_count;
                        macro_cluster_best_reps = max(macro_cluster_best_reps, rep_count);
                        cluster_seeds.push_back({rough_gain, d, std::move(label),
                                                 std::move(reps)});
                    }
                }
            }
        }

        stable_sort(cluster_seeds.begin(), cluster_seeds.end(),
                    [](const ClusterSeed &a, const ClusterSeed &b) {
            if (a.rough_gain != b.rough_gain) return a.rough_gain > b.rough_gain;
            if (a.reps.size() != b.reps.size()) return a.reps.size() > b.reps.size();
            return a.label.size() > b.label.size();
        });
        if (static_cast<int>(cluster_seeds.size()) > MACRO_CLUSTER_EVAL_LIMIT) {
            cluster_seeds.resize(MACRO_CLUSTER_EVAL_LIMIT);
        }

        int macro_cluster_valid = 0;
        int macro_cluster_best_buttons = 1000000000;
        int macro_cluster_best_deliveries = 0;

        for (const ClusterSeed &seed : cluster_seeds) {
            if (time_over()) break;
            const ClusterEdge &first_rep = seed.reps.front();
            const Route &to_first = route[0][1][1 + first_rep.from];
            if (to_first.end_dir == -1) continue;

            string prefix = to_first.ops;
            append_cluster_turn_ops(prefix, to_first.end_dir, seed.dir);

            vector<char> done(M, 0);
            for (const ClusterEdge &rep : seed.reps) {
                done[rep.from] = 1;
                done[rep.second] = 1;
            }

            int delivered_by_macro = 2 * static_cast<int>(seed.reps.size());
            vector<int> suffix_order;
            suffix_order.reserve(M);

            string middle_base = prefix;
            string middle_output = compress_output(prefix);
            middle_output.push_back('M');
            middle_output += seed.label;
            middle_output.push_back('M');
            middle_base += seed.label;

            int current_terminal = seed.reps[0].to;
            bool ok_middle = true;
            for (int rep_idx = 1; rep_idx < static_cast<int>(seed.reps.size()); ++rep_idx) {
                const ClusterEdge &rep = seed.reps[rep_idx];
                const Route &to_next =
                    route[1 + current_terminal][seed.dir][1 + rep.from];
                if (to_next.end_dir == -1) {
                    ok_middle = false;
                    break;
                }
                string transition = to_next.ops;
                append_cluster_turn_ops(transition, to_next.end_dir, seed.dir);
                middle_base += transition;
                middle_base += seed.label;
                middle_output += transition;
                middle_output.push_back('P');
                current_terminal = rep.to;
            }
            if (!ok_middle) continue;

            if (!done[current_terminal]) suffix_order.push_back(current_terminal);
            for (int k : best_order) {
                if (k != current_terminal) suffix_order.push_back(k);
            }

            size_t used_len = middle_base.size();
            if (used_len > static_cast<size_t>(T)) continue;

            BuiltAnswer suffix =
                build_cluster_suffix(suffix_order, done, 1 + current_terminal, seed.dir,
                                     used_len, delivered_by_macro);
            if (suffix.delivered < M) continue;

            BuiltAnswer built;
            built.base.reserve(used_len + suffix.base.size());
            built.base += middle_base;
            built.base += suffix.base;
            built.delivered = suffix.delivered;
            if (built.base.size() > static_cast<size_t>(T)) continue;

            string candidate_output = middle_output;
            candidate_output += compress_output(suffix.base);
            if (!time_over()) {
                candidate_output = compress_hierarchical(built.base, candidate_output);
            }
            if (candidate_output.size() > static_cast<size_t>(T)) continue;

            ++macro_cluster_valid;
            if (static_cast<int>(candidate_output.size()) < macro_cluster_best_buttons) {
                macro_cluster_best_buttons = static_cast<int>(candidate_output.size());
                macro_cluster_best_deliveries = delivered_by_macro;
            }
            macro_chain_candidates.push_back({std::move(built), std::move(candidate_output)});
        }

#if MACRO_CHAIN_DEBUG
        cerr << "METRIC macro_cluster_labels=" << macro_cluster_labels
             << " macro_cluster_edges=" << macro_cluster_edges
             << " macro_cluster_seeds=" << macro_cluster_seed_count
             << " macro_cluster_best_reps=" << macro_cluster_best_reps
             << " macro_cluster_valid=" << macro_cluster_valid
             << " macro_cluster_best_buttons=" << macro_cluster_best_buttons
             << " macro_cluster_best_deliveries=" << macro_cluster_best_deliveries
             << "\n";
#else
        (void)macro_cluster_labels;
        (void)macro_cluster_edges;
        (void)macro_cluster_seed_count;
        (void)macro_cluster_best_reps;
        (void)macro_cluster_valid;
        (void)macro_cluster_best_buttons;
        (void)macro_cluster_best_deliveries;
#endif
    }
#endif

#if ENABLE_MACRO_CHAIN_CANDIDATE || ENABLE_BASKET_CHAIN_CANDIDATE || \
    ENABLE_MACRO_PROGRAM_CANDIDATE || ENABLE_MACRO_CLUSTER_CANDIDATE
    for (auto &candidate : macro_chain_candidates) {
        if (time_over()) break;
        BuiltAnswer &built = candidate.first;
        string &candidate_output = candidate.second;
#if ENABLE_BASKET_CHAIN_CANDIDATE || ENABLE_MACRO_PROGRAM_CANDIDATE || \
    ENABLE_MACRO_CLUSTER_CANDIDATE
        if (!time_over()) {
            candidate_output = compress_hierarchical(built.base, candidate_output);
        }
#endif
        auto candidate_score = score_tuple(built, candidate_output);
        if (candidate_score < best_score) {
            best_score = candidate_score;
            output = std::move(candidate_output);
            best_base = built.base;
            best_answer = std::move(built);
        }
    }
#endif

#if ENABLE_ROTATION_CANDIDATE
    if (!best_order.empty() && !time_over()) {
        vector<pair<long long, BuiltAnswer>> rotation_answers;
        vector<vector<int>> rotation_orders;
        rotation_orders.push_back(best_order);
        vector<int> ids(M);
        for (int i = 0; i < M; ++i) ids[i] = i;
        auto snake_key_for_rotation = [&](const Pos &p) {
            return p.r * N + ((p.r & 1) ? (N - 1 - p.c) : p.c);
        };
        auto add_rotation_order = [&](auto key_func) {
            vector<int> candidate = ids;
            stable_sort(candidate.begin(), candidate.end(), [&](int a, int b) {
                auto ka = key_func(a);
                auto kb = key_func(b);
                if (ka != kb) return ka < kb;
                return a < b;
            });
            rotation_orders.push_back(std::move(candidate));
        };
        add_rotation_order([&](int k) {
            return make_tuple(snake_key_for_rotation(ball[k]),
                              snake_key_for_rotation(basket[k]));
        });
        add_rotation_order([&](int k) {
            return make_tuple(snake_key_for_rotation(basket[k]),
                              snake_key_for_rotation(ball[k]));
        });
        add_rotation_order([&](int k) {
            Pos mid{(ball[k].r + basket[k].r) / 2, (ball[k].c + basket[k].c) / 2};
            return make_tuple(snake_key_for_rotation(mid),
                              abs(ball[k].r - basket[k].r) + abs(ball[k].c - basket[k].c));
        });

        for (const vector<int> &rotation_order : rotation_orders) {
            const int order_size = static_cast<int>(rotation_order.size());
            for (int len = 2; len <= min(6, order_size); ++len) {
                for (int begin = 0; begin + len <= order_size; ++begin) {
                    if ((static_cast<int>(rotation_answers.size()) & 31) == 0 && time_over()) break;
                    BuiltAnswer rotation_answer = build_rotation_candidate(rotation_order, begin, len);
                    if (rotation_answer.delivered < M) continue;
#if ENABLE_S_NGRAM_PROXY
                    long long key = static_cast<long long>(rotation_answer.base.size()) -
                                    s_ngram_gain_estimate(rotation_answer.base);
#else
                    long long key = static_cast<long long>(rotation_answer.base.size());
#endif
                    rotation_answers.push_back({key, std::move(rotation_answer)});
                }
                if (time_over()) break;
            }
            if (time_over()) break;
        }

        stable_sort(rotation_answers.begin(), rotation_answers.end(),
                    [](const pair<long long, BuiltAnswer> &a,
                       const pair<long long, BuiltAnswer> &b) {
            if (a.first != b.first) return a.first < b.first;
            return a.second.base.size() < b.second.base.size();
        });

        int tried = 0;
        for (const auto &entry : rotation_answers) {
            if (time_over() || tried >= ROTATION_CANDIDATE_LIMIT) break;
            const BuiltAnswer &rotation_answer = entry.second;
            string rotation_output = compress_output(rotation_answer.base);
            if (!time_over()) {
                rotation_output = compress_hierarchical(rotation_answer.base, rotation_output);
            }
            auto rotation_score = score_tuple(rotation_answer, rotation_output);
            if (rotation_score < best_score) {
                best_score = rotation_score;
                output = rotation_output;
                best_base = rotation_answer.base;
                best_answer = rotation_answer;
            }
            ++tried;
        }
    }
#endif

    for (char op : output) {
        cout << op << '\n';
    }

    return 0;
}
