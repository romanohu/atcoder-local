#include <algorithm>
#include <array>
#include <iostream>
#include <limits>
#include <queue>
#include <utility>
#include <vector>
using namespace std;

#ifndef ROLLOUT_BOXES
#define ROLLOUT_BOXES 3
#endif

#ifndef ROLLOUT_PATH_LIMIT
#define ROLLOUT_PATH_LIMIT 3
#endif

#ifndef ROLLOUT_PRE_MUL
#define ROLLOUT_PRE_MUL 2
#endif

struct Operation {
    int belt;
    int dir;
};

struct Config {
    int layout;
    int side;
    bool helper;
    bool row_rev;
    bool vertical_rev;
};

struct State {
    vector<int> board;
    vector<int> pos;
    int next_box = 0;
};

struct Candidate {
    vector<Operation> ops;
    State state;
    double score = 0.0;
};

struct Node {
    vector<Operation> first_ops;
    State state;
    int cost = 0;
    double score = 0.0;
};

struct LightPlan {
    State state;
    int cost = 0;
    double score = 0.0;
};

class Solver {
public:
    enum : int {
        N = 20,
        TOTAL = N * N,
        EXIT_COL = N / 2,
        EXIT_ID = EXIT_COL,
        MAX_OPS = 100000
    };

    explicit Solver(const vector<int> &initial_board, Config cfg)
        : initial(initial_board), config(cfg) {
        build_belts();
        build_indexes();
    }

    Candidate solve() {
        Candidate best = solve_box_beam();
        if (config.layout == 1) {
            Candidate op_beam = solve_operation_beam();
            if (op_beam.score < best.score) return op_beam;
        }
        return best;
    }

    Candidate solve_box_beam() {
        State state;
        state.board = initial;
        state.pos.assign(TOTAL, -1);
        for (int id = 0; id < TOTAL; id++) {
            state.pos[state.board[id]] = id;
        }
        remove_if_ready(state);

        vector<Operation> all_ops;
        while (state.next_box < TOTAL && (int)all_ops.size() < MAX_OPS) {
            vector<Operation> next_ops = choose_next_ops(state);
            if (next_ops.empty()) break;
            for (auto op : next_ops) {
                if ((int)all_ops.size() >= MAX_OPS) break;
                rotate_belt(state, op.belt, op.dir, nullptr);
                all_ops.push_back(op);
                if (state.next_box == TOTAL) break;
            }
        }

        Candidate result;
        result.ops = all_ops;
        result.state = state;
        result.score = (state.next_box == TOTAL ? all_ops.size()
                                                : 1000000.0 + (TOTAL - state.next_box) * 1000.0);
        return result;
    }

    void print_answer(const vector<Operation> &ops) const {
        cout << belts.size() << '\n';
        for (const auto &belt : belts) {
            cout << belt.size();
            for (int id : belt) {
                cout << ' ' << id / N << ' ' << id % N;
            }
            cout << '\n';
        }

        cout << ops.size() << '\n';
        for (auto op : ops) {
            cout << op.belt << ' ' << op.dir << '\n';
        }
    }

    bool valid_layout() const {
        vector<int> cnt(TOTAL, 0);
        for (const auto &belt : belts) {
            if ((int)belt.size() < 2) return false;
            vector<int> seen(TOTAL, 0);
            for (int id : belt) {
                if (id < 0 || id >= TOTAL) return false;
                if (seen[id]) return false;
                seen[id] = 1;
                cnt[id]++;
            }
            for (int i = 0; i < (int)belt.size(); i++) {
                int a = belt[i];
                int b = belt[(i + 1) % belt.size()];
                int ar = a / N, ac = a % N;
                int br = b / N, bc = b % N;
                if (abs(ar - br) + abs(ac - bc) != 1) return false;
            }
        }
        for (int v : cnt) {
            if (v > 2) return false;
        }
        return true;
    }

private:
    vector<int> initial;
    Config config;
    vector<vector<int>> belts;
    vector<vector<int>> belt_index;
    vector<vector<int>> cell_belts;
    vector<vector<vector<int>>> common_cells;
    vector<vector<int>> graph;
    vector<vector<vector<int>>> paths_from_cell;
    vector<int> estimate_from_cell;
    vector<int> exit_belts;

    static int cell_id(int r, int c) {
        return r * N + c;
    }

    void add_horizontal_loop(int r, int l, int rr) {
        vector<int> belt;
        if (!config.row_rev) {
            for (int c = l; c <= rr; c++) belt.push_back(cell_id(r, c));
            for (int c = rr; c >= l; c--) belt.push_back(cell_id(r + 1, c));
        } else {
            for (int c = rr; c >= l; c--) belt.push_back(cell_id(r, c));
            for (int c = l; c <= rr; c++) belt.push_back(cell_id(r + 1, c));
        }
        belts.push_back(belt);
    }

    void add_vertical_pair_loop(int c0, int c1) {
        vector<int> belt;
        for (int r = 0; r < N; r++) belt.push_back(cell_id(r, c0));
        for (int r = N - 1; r >= 0; r--) belt.push_back(cell_id(r, c1));
        if (config.vertical_rev) reverse(belt.begin(), belt.end());
        belts.push_back(belt);
    }

    void add_vertical_loop(int other_col) {
        add_vertical_pair_loop(EXIT_COL, other_col);
    }

    void add_perimeter_loop() {
        vector<int> belt;
        for (int c = 0; c < N; c++) belt.push_back(cell_id(0, c));
        for (int r = 1; r < N; r++) belt.push_back(cell_id(r, N - 1));
        for (int c = N - 2; c >= 0; c--) belt.push_back(cell_id(N - 1, c));
        for (int r = N - 2; r >= 1; r--) belt.push_back(cell_id(r, 0));
        belts.push_back(belt);
    }

    void add_helper_loop(int helper_col) {
        vector<int> belt;
        if (helper_col < EXIT_COL) {
            belt = {cell_id(0, helper_col), cell_id(0, EXIT_COL),
                    cell_id(1, EXIT_COL), cell_id(1, helper_col)};
        } else {
            belt = {cell_id(0, EXIT_COL), cell_id(0, helper_col),
                    cell_id(1, helper_col), cell_id(1, EXIT_COL)};
        }
        belts.push_back(belt);
    }

    void build_belts() {
        int other_col = EXIT_COL + config.side;
        int helper_col = EXIT_COL - config.side;

        if (config.layout == 1) {
            for (int r = 0; r < N; r += 2) add_horizontal_loop(r, 0, N - 1);
            for (int c = 0; c < N; c += 2) add_vertical_pair_loop(c, c + 1);
        } else if (config.layout == 2) {
            for (int r = 0; r < N; r += 2) add_horizontal_loop(r, 0, N - 1);
            for (int c = 1; c + 1 < N; c += 2) add_vertical_pair_loop(c, c + 1);
        } else if (config.layout == 3) {
            add_perimeter_loop();
            for (int r = 1; r + 1 < N; r += 2) add_horizontal_loop(r, 0, N - 1);
            for (int c = 1; c + 1 < N; c += 2) add_vertical_pair_loop(c, c + 1);
        } else if (config.layout == 4) {
            add_perimeter_loop();
            for (int r = 1; r + 1 < N; r += 2) add_horizontal_loop(r, 1, N - 2);
            for (int c = 0; c < N; c += 2) add_vertical_pair_loop(c, c + 1);
        } else if (config.layout == 5) {
            for (int r = 0; r < N; r += 2) add_horizontal_loop(r, 0, N - 1);
            for (int c : {1, 3, 5, 7, 10, 13, 15, 17}) {
                add_vertical_pair_loop(c, c + 1);
            }
        } else if (config.helper) {
            add_horizontal_loop(0, 0, EXIT_COL - 1);
            add_horizontal_loop(0, EXIT_COL + 1, N - 1);
            for (int r = 2; r < N; r += 2) add_horizontal_loop(r, 0, N - 1);
            add_vertical_loop(other_col);
            add_helper_loop(helper_col);
        } else {
            for (int r = 0; r < N; r += 2) add_horizontal_loop(r, 0, N - 1);
            add_vertical_loop(other_col);
        }
    }

    void build_indexes() {
        int m = belts.size();
        belt_index.assign(m, vector<int>(TOTAL, -1));
        cell_belts.assign(TOTAL, {});

        for (int b = 0; b < m; b++) {
            for (int i = 0; i < (int)belts[b].size(); i++) {
                int id = belts[b][i];
                belt_index[b][id] = i;
                cell_belts[id].push_back(b);
            }
        }
        exit_belts = cell_belts[EXIT_ID];

        common_cells.assign(m, vector<vector<int>>(m));
        graph.assign(m, {});
        for (int a = 0; a < m; a++) {
            for (int b = 0; b < m; b++) {
                if (a == b) continue;
                for (int id = 0; id < TOTAL; id++) {
                    if (belt_index[a][id] != -1 && belt_index[b][id] != -1) {
                        common_cells[a][b].push_back(id);
                    }
                }
                if (!common_cells[a][b].empty()) graph[a].push_back(b);
            }
        }
        build_route_cache();
    }

    void remove_if_ready(State &state) const {
        while (state.next_box < TOTAL && state.board[EXIT_ID] == state.next_box) {
            state.pos[state.next_box] = -1;
            state.board[EXIT_ID] = -1;
            state.next_box++;
        }
    }

    void rotate_belt(State &state, int belt, int dir, vector<Operation> *ops) const {
        const vector<int> &cells = belts[belt];
        int len = cells.size();
        array<int, TOTAL> old_values;

        for (int i = 0; i < len; i++) old_values[i] = state.board[cells[i]];
        for (int i = 0; i < len; i++) {
            int ni = (i + dir + len) % len;
            int to = cells[ni];
            int value = old_values[i];
            state.board[to] = value;
            if (value != -1) state.pos[value] = to;
        }

        if (ops != nullptr) ops->push_back({belt, dir});
        remove_if_ready(state);
    }

    vector<vector<int>> compute_grid_paths(int start_cell) const {
        vector<vector<int>> paths;
        const vector<int> &starts = cell_belts[start_cell];
        const vector<int> &exits = cell_belts[EXIT_ID];

        for (int start : starts) {
            if (belt_index[start][EXIT_ID] != -1) paths.push_back({start});
            for (int exit_belt : exits) {
                if (start != exit_belt && !common_cells[start][exit_belt].empty()) {
                    paths.push_back({start, exit_belt});
                }
            }
        }

        sort(paths.begin(), paths.end());
        paths.erase(unique(paths.begin(), paths.end()), paths.end());
        return paths;
    }

    vector<vector<int>> compute_paths(int start_cell) const {
        if (config.layout == 1) return compute_grid_paths(start_cell);

        vector<int> starts = cell_belts[start_cell];
        vector<int> exits = cell_belts[EXIT_ID];
        int m = belts.size();

        vector<int> dist(m, numeric_limits<int>::max());
        queue<int> que;
        for (int b : starts) {
            dist[b] = 0;
            que.push(b);
        }
        while (!que.empty()) {
            int v = que.front();
            que.pop();
            for (int to : graph[v]) {
                if (dist[to] > dist[v] + 1) {
                    dist[to] = dist[v] + 1;
                    que.push(to);
                }
            }
        }

        int best = numeric_limits<int>::max();
        for (int b : exits) best = min(best, dist[b]);
        vector<vector<int>> paths;
        if (best == numeric_limits<int>::max()) return paths;

        vector<int> path;
        vector<int> used(m, 0);
        int limit = best + 1;

        auto dfs = [&](auto &&self, int belt, int depth) -> void {
            path.push_back(belt);
            used[belt] = 1;

            if (belt_index[belt][EXIT_ID] != -1 && depth >= best) {
                paths.push_back(path);
            }
            if (depth < limit) {
                for (int to : graph[belt]) {
                    if (!used[to] && dist[to] <= best + 1) self(self, to, depth + 1);
                }
            }

            used[belt] = 0;
            path.pop_back();
        };

        for (int b : starts) dfs(dfs, b, 0);
        sort(paths.begin(), paths.end());
        paths.erase(unique(paths.begin(), paths.end()), paths.end());
        return paths;
    }

    void build_route_cache() {
        paths_from_cell.assign(TOTAL, {});
        estimate_from_cell.assign(TOTAL, 1000);
        for (int id = 0; id < TOTAL; id++) {
            paths_from_cell[id] = compute_paths(id);
            sort(paths_from_cell[id].begin(), paths_from_cell[id].end(),
                 [&](const vector<int> &a, const vector<int> &b) {
                     int ea = estimate_path(a, 0, id);
                     int eb = estimate_path(b, 0, id);
                     if (ea != eb) return ea < eb;
                     return a < b;
                 });
            int best = numeric_limits<int>::max();
            for (const auto &path : paths_from_cell[id]) {
                best = min(best, estimate_path(path, 0, id));
            }
            if (best != numeric_limits<int>::max()) estimate_from_cell[id] = best;
        }
    }

    vector<pair<int, int>> rotation_options(int belt, int current_cell,
                                            const vector<int> &goal_cells,
                                            int slack) const {
        int len = belts[belt].size();
        int cur = belt_index[belt][current_cell];
        int best = numeric_limits<int>::max();
        vector<pair<int, int>> raw;

        for (int goal_cell : goal_cells) {
            int goal = belt_index[belt][goal_cell];
            int plus = (goal - cur + len) % len;
            int minus = (cur - goal + len) % len;
            best = min(best, min(plus, minus));
            raw.push_back({plus, 1});
            raw.push_back({minus, -1});
        }

        sort(raw.begin(), raw.end());
        raw.erase(unique(raw.begin(), raw.end()), raw.end());

        vector<pair<int, int>> result;
        for (auto [steps, dir] : raw) {
            if (steps <= best + slack) result.push_back({steps, dir});
        }
        if ((int)result.size() > 8) result.resize(8);
        return result;
    }

    int min_distance_between(int belt, int current_cell, const vector<int> &goal_cells) const {
        int len = belts[belt].size();
        int cur = belt_index[belt][current_cell];
        int best = numeric_limits<int>::max();
        for (int goal_cell : goal_cells) {
            int goal = belt_index[belt][goal_cell];
            int plus = (goal - cur + len) % len;
            int minus = (cur - goal + len) % len;
            best = min(best, min(plus, minus));
        }
        return best;
    }

    int estimate_path(const vector<int> &path, int stage, int current_cell) const {
        int belt = path[stage];
        vector<int> goals;
        if (stage + 1 == (int)path.size()) {
            goals.push_back(EXIT_ID);
        } else {
            goals = common_cells[belt][path[stage + 1]];
        }

        int best = numeric_limits<int>::max();
        for (int goal_cell : goals) {
            int d = min_distance_between(belt, current_cell, {goal_cell});
            if (stage + 1 < (int)path.size()) {
                d += estimate_path(path, stage + 1, goal_cell);
            }
            best = min(best, d);
        }
        return best;
    }

    int estimate_box_distance(const State &state, int box) const {
        if (box >= TOTAL || state.pos[box] == -1) return 0;
        return estimate_from_cell[state.pos[box]];
    }

    int steps_to_exit_on_belt(int belt, int cell, int dir) const {
        int len = belts[belt].size();
        int cur = belt_index[belt][cell];
        int goal = belt_index[belt][EXIT_ID];
        if (dir == 1) return (goal - cur + len) % len;
        return (cur - goal + len) % len;
    }

    double exit_queue_bonus(const State &state) const {
        double best = 0.0;

        for (int belt : exit_belts) {
            for (int dir : {-1, 1}) {
                int prev_steps = -1;
                int prefix = 0;
                double reward = 0.0;

                for (int offset = 0; offset < 14; offset++) {
                    int box = state.next_box + offset;
                    if (box >= TOTAL || state.pos[box] == -1) break;
                    int cell = state.pos[box];
                    if (belt_index[belt][cell] == -1) break;

                    int steps = steps_to_exit_on_belt(belt, cell, dir);
                    if (offset == 0) {
                        reward += max(0, 18 - steps) * 0.20;
                    } else {
                        int gap = steps - prev_steps;
                        if (gap <= 0 || gap > 12) break;
                        reward += 3.0 + (12 - gap) * 0.18;
                    }

                    prev_steps = steps;
                    prefix++;
                }

                reward += prefix * prefix * 0.75;
                best = max(best, reward);
            }
        }

        return best;
    }

    double heuristic(const State &state) const {
        static const array<double, 8> weight = {0.50, 0.25, 0.14, 0.08, 0.05, 0.03, 0.02, 0.01};
        double value = 0.0;
        for (int i = 0; i < (int)weight.size(); i++) {
            value += weight[i] * estimate_box_distance(state, state.next_box + i);
        }
        return value - exit_queue_bonus(state);
    }

    LightPlan build_greedy_light_plan(const State &state,
                                      const vector<int> &path,
                                      int target) const {
        State cur = state;
        int cost = 0;

        for (int stage = 0; stage < (int)path.size() && cur.next_box == target; stage++) {
            int belt = path[stage];
            int current_cell = cur.pos[target];
            vector<int> goals;
            if (stage + 1 == (int)path.size()) {
                goals.push_back(EXIT_ID);
            } else {
                goals = common_cells[belt][path[stage + 1]];
            }

            int len = belts[belt].size();
            int cur_index = belt_index[belt][current_cell];
            int best_steps = numeric_limits<int>::max();
            int best_dir = 1;

            for (int goal_cell : goals) {
                int goal_index = belt_index[belt][goal_cell];
                int plus = (goal_index - cur_index + len) % len;
                int minus = (cur_index - goal_index + len) % len;
                if (plus < best_steps) {
                    best_steps = plus;
                    best_dir = 1;
                }
                if (minus < best_steps) {
                    best_steps = minus;
                    best_dir = -1;
                }
            }

            if (best_steps == numeric_limits<int>::max()) {
                return {state, 1000000, 1000000.0};
            }

            for (int step = 0; step < best_steps && cur.next_box == target; step++) {
                rotate_belt(cur, belt, best_dir, nullptr);
                cost++;
            }
        }

        LightPlan plan;
        plan.state = cur;
        plan.cost = cost;
        plan.score = cost + heuristic(cur);
        return plan;
    }

    vector<LightPlan> light_current_box_plans(const State &state, int path_limit) const {
        if (state.next_box >= TOTAL) return {};
        int target = state.next_box;
        int current_cell = state.pos[target];
        const vector<vector<int>> &paths = paths_from_cell[current_cell];
        vector<LightPlan> plans;

        for (int i = 0; i < (int)paths.size() && i < path_limit; i++) {
            LightPlan plan = build_greedy_light_plan(state, paths[i], target);
            if (plan.state.next_box != target) plans.push_back(plan);
        }

        sort(plans.begin(), plans.end(), [](const LightPlan &a, const LightPlan &b) {
            if (a.score != b.score) return a.score < b.score;
            return a.cost < b.cost;
        });
        return plans;
    }

    double rollout_score(const State &state, int fixed_cost) const {
        State cur = state;
        int rollout_ops = 0;
        int start_box = cur.next_box;
        int want = min(ROLLOUT_BOXES, TOTAL - start_box);

        for (int i = 0; i < want && cur.next_box < TOTAL; i++) {
            vector<LightPlan> plans = light_current_box_plans(cur, ROLLOUT_PATH_LIMIT);
            if (plans.empty()) break;
            cur = plans[0].state;
            rollout_ops += plans[0].cost;
        }

        int progressed = cur.next_box - start_box;
        int missed = want - progressed;
        return fixed_cost + 0.65 * heuristic(state) + 0.25 * rollout_ops
               + 0.20 * heuristic(cur) + 25.0 * missed;
    }

    void add_action(vector<Operation> &actions, int belt, int dir) const {
        if (belt < 0 || belt >= (int)belts.size()) return;
        actions.push_back({belt, dir});
    }

    vector<Operation> candidate_actions(const State &state) const {
        vector<Operation> actions;
        int target = state.next_box;

        for (int belt : exit_belts) {
            add_action(actions, belt, 1);
            add_action(actions, belt, -1);
        }

        for (int offset = 0; offset < 6; offset++) {
            int box = target + offset;
            if (box >= TOTAL || state.pos[box] == -1) continue;
            int cell = state.pos[box];
            const auto &paths = paths_from_cell[cell];

            for (int pi = 0; pi < (int)paths.size() && pi < 2; pi++) {
                const vector<int> &path = paths[pi];
                int belt = path[0];
                vector<int> goals;
                if ((int)path.size() == 1) {
                    goals.push_back(EXIT_ID);
                } else {
                    goals = common_cells[belt][path[1]];
                }

                int cur = belt_index[belt][cell];
                int len = belts[belt].size();
                int best = numeric_limits<int>::max();
                vector<pair<int, int>> opts;
                for (int goal_cell : goals) {
                    int goal = belt_index[belt][goal_cell];
                    int plus = (goal - cur + len) % len;
                    int minus = (cur - goal + len) % len;
                    best = min(best, min(plus, minus));
                    opts.push_back({plus, 1});
                    opts.push_back({minus, -1});
                }

                for (auto [steps, dir] : opts) {
                    if (steps > 0 && steps <= best + 3) add_action(actions, belt, dir);
                }
            }
        }

        sort(actions.begin(), actions.end(), [](const Operation &a, const Operation &b) {
            if (a.belt != b.belt) return a.belt < b.belt;
            return a.dir < b.dir;
        });
        actions.erase(unique(actions.begin(), actions.end(), [](const Operation &a, const Operation &b) {
            return a.belt == b.belt && a.dir == b.dir;
        }), actions.end());
        if ((int)actions.size() > 14) actions.resize(14);
        return actions;
    }

    double operation_score(const State &state, int start_next, int cost) const {
        int done = state.next_box - start_next;
        return -1200.0 * done + cost + 8.0 * heuristic(state);
    }

    vector<Operation> choose_operation_chunk(const State &state) const {
        const int WIDTH = 8;
        const int HORIZON = 8;
        const int COMMIT = 6;
        int start_next = state.next_box;

        vector<Node> beam;
        Node root;
        root.state = state;
        root.cost = 0;
        root.score = operation_score(root.state, start_next, 0);
        beam.push_back(root);

        for (int depth = 0; depth < HORIZON; depth++) {
            vector<Node> next_beam;
            for (const Node &node : beam) {
                if (node.state.next_box >= TOTAL) {
                    next_beam.push_back(node);
                    continue;
                }

                vector<Operation> actions = candidate_actions(node.state);
                for (Operation op : actions) {
                    Node nxt;
                    nxt.first_ops = node.first_ops;
                    nxt.state = node.state;
                    nxt.cost = node.cost + 1;
                    rotate_belt(nxt.state, op.belt, op.dir, nullptr);
                    if ((int)nxt.first_ops.size() < COMMIT) nxt.first_ops.push_back(op);
                    nxt.score = operation_score(nxt.state, start_next, nxt.cost);
                    next_beam.push_back(nxt);
                }
            }

            sort(next_beam.begin(), next_beam.end(), [](const Node &a, const Node &b) {
                if (a.score != b.score) return a.score < b.score;
                return a.cost < b.cost;
            });
            if ((int)next_beam.size() > WIDTH) next_beam.resize(WIDTH);
            beam.swap(next_beam);
            if (beam.empty()) break;
        }

        if (beam.empty()) return {};
        return beam.front().first_ops;
    }

    Candidate solve_operation_beam() {
        State state;
        state.board = initial;
        state.pos.assign(TOTAL, -1);
        for (int id = 0; id < TOTAL; id++) {
            state.pos[state.board[id]] = id;
        }
        remove_if_ready(state);

        vector<Operation> all_ops;
        int stuck = 0;
        while (state.next_box < TOTAL && (int)all_ops.size() < MAX_OPS) {
            int before = state.next_box;
            vector<Operation> chunk = choose_operation_chunk(state);
            if (chunk.empty()) break;

            for (Operation op : chunk) {
                if ((int)all_ops.size() >= MAX_OPS) break;
                rotate_belt(state, op.belt, op.dir, nullptr);
                all_ops.push_back(op);
                if (state.next_box > before) break;
            }

            if (state.next_box == before) {
                stuck++;
                if (stuck > 800) break;
            } else {
                stuck = 0;
            }
        }

        Candidate result;
        result.ops = all_ops;
        result.state = state;
        result.score = (state.next_box == TOTAL ? all_ops.size()
                                                : 1000000.0 + (TOTAL - state.next_box) * 1000.0);
        return result;
    }

    void build_plans_for_path(const vector<int> &path, int stage, int target,
                              const State &state, vector<Operation> &ops,
                              vector<Candidate> &out) const {
        if (state.next_box != target) {
            Candidate cand;
            cand.ops = ops;
            cand.state = state;
            cand.score = ops.size() + heuristic(state);
            out.push_back(cand);
            return;
        }
        if (stage >= (int)path.size()) return;

        int belt = path[stage];
        int current_cell = state.pos[target];
        vector<int> goals;
        if (stage + 1 == (int)path.size()) {
            goals.push_back(EXIT_ID);
        } else {
            goals = common_cells[belt][path[stage + 1]];
        }

        int slack = (stage + 1 == (int)path.size() ? 10 : 4);
        vector<pair<int, int>> options = rotation_options(belt, current_cell, goals, slack);
        for (auto [steps, dir] : options) {
            State next_state = state;
            int old_size = ops.size();
            for (int i = 0; i < steps && next_state.next_box == target; i++) {
                rotate_belt(next_state, belt, dir, &ops);
            }
            build_plans_for_path(path, stage + 1, target, next_state, ops, out);
            ops.resize(old_size);
        }
    }

    vector<Candidate> current_box_plans(const State &state, int limit) const {
        if (state.next_box >= TOTAL) return {};
        int target = state.next_box;
        const vector<vector<int>> &paths = paths_from_cell[state.pos[target]];
        vector<Candidate> plans;
        vector<Operation> ops;

        for (const auto &path : paths) {
            build_plans_for_path(path, 0, target, state, ops, plans);
        }

        sort(plans.begin(), plans.end(), [](const Candidate &a, const Candidate &b) {
            if (a.score != b.score) return a.score < b.score;
            return a.ops.size() < b.ops.size();
        });
        if ((int)plans.size() > limit) plans.resize(limit);
        return plans;
    }

    vector<Operation> choose_next_ops(const State &state) const {
        bool light_grid = (config.layout >= 2);
        int width = (light_grid ? 6 : 10);
        int depth_limit = (light_grid ? 2 : 3);
        int first_limit = (light_grid ? 14 : 20);
        int next_limit = (light_grid ? 8 : 10);
        if (config.layout == 3) {
            width = 3;
            first_limit = 6;
            next_limit = 4;
        }
        vector<Candidate> first = current_box_plans(state, first_limit);
        if (first.empty()) return {};

        vector<Node> beam;
        for (const auto &cand : first) {
            Node node;
            node.first_ops = cand.ops;
            node.state = cand.state;
            node.cost = cand.ops.size();
            node.score = node.cost + heuristic(node.state);
            beam.push_back(node);
        }

        auto trim = [&]() {
            sort(beam.begin(), beam.end(), [](const Node &a, const Node &b) {
                if (a.score != b.score) return a.score < b.score;
                return a.cost < b.cost;
            });
            int pre_limit = width * ROLLOUT_PRE_MUL;
            if ((int)beam.size() > pre_limit) beam.resize(pre_limit);

            for (Node &node : beam) {
                node.score = rollout_score(node.state, node.cost);
            }
            sort(beam.begin(), beam.end(), [](const Node &a, const Node &b) {
                if (a.score != b.score) return a.score < b.score;
                return a.cost < b.cost;
            });
            if ((int)beam.size() > width) beam.resize(width);
        };
        trim();

        for (int depth = 1; depth < depth_limit; depth++) {
            vector<Node> next_beam;
            for (const auto &node : beam) {
                if (node.state.next_box >= TOTAL) {
                    next_beam.push_back(node);
                    continue;
                }
                vector<Candidate> plans = current_box_plans(node.state, next_limit);
                for (const auto &plan : plans) {
                    Node nxt;
                    nxt.first_ops = node.first_ops;
                    nxt.state = plan.state;
                    nxt.cost = node.cost + (int)plan.ops.size();
                    nxt.score = nxt.cost + heuristic(nxt.state);
                    next_beam.push_back(nxt);
                }
            }
            beam.swap(next_beam);
            trim();
        }

        return beam.front().first_ops;
    }
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int n;
    cin >> n;

    vector<int> initial(Solver::TOTAL);
    for (int r = 0; r < Solver::N; r++) {
        for (int c = 0; c < Solver::N; c++) {
            cin >> initial[r * Solver::N + c];
        }
    }

    vector<Config> configs;
    configs.push_back({2, 1, false, false, false});

    bool found = false;
    Config best_config = configs[0];
    vector<Operation> best_ops;
    double best_score = numeric_limits<double>::infinity();

    for (Config cfg : configs) {
        Solver solver(initial, cfg);
        if (!solver.valid_layout()) continue;
        Candidate cand = solver.solve();
        if (!found || cand.score < best_score) {
            best_config = cfg;
            best_ops = cand.ops;
            best_score = cand.score;
            found = true;
        }
    }

    Solver best_solver(initial, best_config);
    best_solver.print_answer(best_ops);
    return 0;
}
