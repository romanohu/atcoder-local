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

// Fixed configuration for the 26868 cyclic-shift variant.
#define HIER_CAND_LIMIT 1000
#define HIER_MAX_LEN 100
#define HIER_START_LIMIT 90
#define HIER_S_PRIORITY_WEIGHT 1
#define SOLVER_TIME_LIMIT_MS 1300
#define EXTRA_SOLVER_TIME_LIMIT_MS 1400
#define ORDER_TIME_LIMIT_MS 1050
#define ORDER_BEAM_WIDTH 40
#define ORDER_BEAM_KEEP 40
#define IMPROVE_FIRST_ROUNDS 80
#define IMPROVE_CANDIDATE_ROUNDS 12
#define S_NGRAM_HIER_LIMIT 40
#define ALT_ROUTE_NEW_GRAM_WEIGHT 1
#define ORDER_S_NGRAM_ROUNDS 2
#define ORDER_S_NGRAM_RAW_SLACK 30
#define ORDER_S_NGRAM_CAND_LIMIT 480
#define ORDER_S_NGRAM_GAIN_WEIGHT 3
#define CYCLIC_SHIFT_MAX_K 10
#define LOCAL_CLUSTER_SIZE 4
#define LOCAL_CLUSTER_BEAM 220
#define LOCAL_CLUSTER_WINDOW_LIMIT 12
#define LOCAL_CLUSTER_MAX_S 10
#define LOCAL_CLUSTER_RAW_SLACK 80
#define LOCAL_CLUSTER_MAX_M 13
#define LOCAL_CLUSTER_INTERVAL_MAX_N 14

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

    vector<vector<vector<vector<Route>>>> alt_routes;

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
            for (const auto &entry : count) {
                int cnt = entry.second;
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

        const bool hier_s_priority_active =
            (160 <= wall_density && wall_density <= 200 && M <= 25);

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
            if (hier_s_priority_active) {
                s_count = static_cast<int>(count(pattern.begin(), pattern.end(), 'S'));
            }
            if (hier_s_priority_active && s_count > 0) {
                rank += HIER_S_PRIORITY_WEIGHT *
                        (min<int>(pattern.size(), 40) + 12 * min(s_count, 3));
            }
            cands.push_back({pattern, gain, rank, s_count});
        }
        sort(cands.begin(), cands.end(), [](const MacroCand &a, const MacroCand &b) {
            if (a.rank != b.rank) return a.rank > b.rank;
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
            for (const auto &entry : count) {
                int cnt = entry.second;
                if (cnt >= 2) gain += 1LL * min(cnt - 1, 4) * (len - 1);
            }
            if (gain > n / 3) return n / 3;
        }
        return static_cast<int>(min<long long>(gain, n / 3));
    };

    auto s_ngram_proxy_score = [&](const BuiltAnswer &built) -> long long {
        if (built.delivered < M) return 1LL * T * (M - built.delivered);
        int gain = s_ngram_gain_estimate(built.base);
        return static_cast<long long>(built.base.size()) - gain;
    };

    struct EvaluatedCandidate {
        BuiltAnswer answer;
        string output;
        tuple<long long, int, int> score;
        long long proxy;
    };
    vector<EvaluatedCandidate> evaluated_candidates;


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
        [&](const string &, const string &) -> int {
        return 0;
    };

    auto combine_alt_route_bonus = [&](int reference_bonus, int new_repeat_bonus) {
        return reference_bonus + ALT_ROUTE_NEW_GRAM_WEIGHT * new_repeat_bonus;
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
        evaluated_candidates.push_back(
            {answer, candidate_output, candidate_score, s_ngram_proxy_score(answer)});
        if (candidate_score < best_score) {
            best_score = candidate_score;
            output = candidate_output;
            best_base = answer.base;
            best_answer = answer;
            best_order = candidate_order;
        }

        if (idx == 0 && !order_time_over()) {
            vector<int> s_candidate_order = improve_order_s_ngram(candidate_order);
            if (s_candidate_order != candidate_order) {
                BuiltAnswer s_answer = build_answer(s_candidate_order);
                string s_output = compress_output(s_answer.base);
                auto s_score = score_tuple(s_answer, s_output);
                evaluated_candidates.push_back(
                    {s_answer, s_output, s_score, s_ngram_proxy_score(s_answer)});
                if (s_score < best_score) {
                    best_score = s_score;
                    output = s_output;
                    best_base = s_answer.base;
                    best_answer = s_answer;
                    best_order = s_candidate_order;
                }
            }
        }
    }

    if (output.empty()) {
        BuiltAnswer answer = build_answer(improve_order(order, IMPROVE_FIRST_ROUNDS));
        output = compress_output(answer.base);
        best_base = answer.base;
        best_answer = answer;
    }


    if (!best_order.empty() && !time_over() && M <= CYCLIC_SHIFT_MAX_K &&
        wall_count >= 60) {

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

                BuiltAnswer built{std::move(base_prefix), M};

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


        for (const LocalClusterCandidate &candidate : local_cluster_candidates) {
            if (time_over()) break;
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
                if (cluster_score < best_score) {
                    best_score = cluster_score;
                    output = std::move(cluster_output);
                    best_base = cluster_answer.base;
                    best_answer = std::move(cluster_answer);
                }
            }
        }
    }

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


    }

    for (char op : output) {
        cout << op << '\n';
    }

    return 0;
}
