#include "local_search_cm.h"
#include <algorithm>
#include <utility>
#include <vector>

namespace {

enum class MType { Insert, Remove, EdgeSwap };

// Move parametryzowany pozycjami w cyklu (a, b zależne od typu) — bo i tak
// po każdej aplikacji przebudowujemy stan, więc niestabilność pozycji
// nam nie szkodzi (cały ruch wybieramy i aplikujemy w jednej iteracji).
struct CMove {
    MType type;
    int a, b;
    int delta;
};

struct CState {
    std::vector<int>  pos;       // pos[v] = indeks w cyklu lub -1
    std::vector<char> in_cycle;  // 0/1
};

CState build_state(const Solution& sol, int n_total) {
    CState s;
    s.pos.assign(n_total, -1);
    s.in_cycle.assign(n_total, 0);
    for (int i = 0; i < sol.size(); i++) {
        int v = sol.cycle[i];
        s.pos[v] = i;
        s.in_cycle[v] = 1;
    }
    return s;
}

void apply_move(Solution& sol, CState& state, const CMove& m, int n_total) {
    switch (m.type) {
        case MType::Insert:
            sol.cycle.insert(sol.cycle.begin() + m.a + 1, m.b);
            break;
        case MType::Remove:
            sol.cycle.erase(sol.cycle.begin() + m.a);
            break;
        case MType::EdgeSwap:
            std::reverse(sol.cycle.begin() + m.a + 1,
                         sol.cycle.begin() + m.b + 1);
            break;
    }
    state = build_state(sol, n_total);
}

bool list_contains(const std::vector<int>& v, int x) {
    for (int y : v) if (y == x) return true;
    return false;
}

} // namespace

std::vector<std::vector<int>> compute_nearest(const Instance& inst, int k) {
    int n = inst.n();
    std::vector<std::vector<int>> result(n);
    std::vector<std::pair<int,int>> tmp; // (dist, vertex)
    tmp.reserve(n);

    for (int i = 0; i < n; i++) {
        tmp.clear();
        for (int j = 0; j < n; j++) {
            if (j == i) continue;
            tmp.emplace_back(inst.dist[i][j], j);
        }
        int kk = std::min(k, static_cast<int>(tmp.size()));
        std::partial_sort(tmp.begin(), tmp.begin() + kk, tmp.end());
        result[i].reserve(kk);
        for (int t = 0; t < kk; t++) result[i].push_back(tmp[t].second);
    }
    return result;
}

Solution local_search_cm(const Instance& inst, Solution sol,
                         const std::vector<std::vector<int>>& nearest,
                         const ObjectiveConfig& config) {
    int n_total = inst.n();
    CState state = build_state(sol, n_total);

    auto eval_edge = [&](int i, int j) -> int {
        int m = sol.size();
        int a = sol.cycle[i];
        int b = sol.cycle[(i + 1) % m];
        int c = sol.cycle[j];
        int d = sol.cycle[(j + 1) % m];
        return inst.dist[a][b] + inst.dist[c][d]
             - inst.dist[a][c] - inst.dist[b][d];
    };
    auto eval_insert = [&](int pos, int v) -> int {
        int m = sol.size();
        int a = sol.cycle[pos];
        int b = sol.cycle[(pos + 1) % m];
        return config.node_value_sign * inst.nodes[v].profit
             - (inst.dist[a][v] + inst.dist[v][b] - inst.dist[a][b]);
    };
    auto eval_remove = [&](int i) -> int {
        int m = sol.size();
        int v = sol.cycle[i];
        int p = sol.cycle[(i - 1 + m) % m];
        int q = sol.cycle[(i + 1) % m];
        return -config.node_value_sign * inst.nodes[v].profit
             + inst.dist[p][v] + inst.dist[v][q]
             - inst.dist[p][q];
    };

    while (true) {
        int m = sol.size();
        int best_delta = 0;
        CMove best{};
        bool found = false;

        // Pętla sterowana listami sąsiadów: dla każdej krawędzi kandydackiej
        // (v_id, u) generujemy ruchy, które ją wprowadzają do rozwiązania.
        for (int v_id = 0; v_id < n_total; v_id++) {
            for (int u : nearest[v_id]) {

                if (state.in_cycle[v_id] && state.in_cycle[u]) {
                    // EdgeSwap — dwa 2-opty wprowadzające krawędź (v_id, u).
                    if (m < 4) continue;
                    int pv = state.pos[v_id];
                    int pu = state.pos[u];

                    // Wariant A: 2-opt na pozycjach (pv, pu) — dodaje krawędzie
                    // (v_id, u) i (next(v_id), next(u)).
                    {
                        int i = pv, j = pu;
                        if (i > j) std::swap(i, j);
                        if (j - i >= 2 && !(i == 0 && j == m - 1)) {
                            int d = eval_edge(i, j);
                            if (d > best_delta) {
                                best_delta = d;
                                best = {MType::EdgeSwap, i, j, d};
                                found = true;
                            }
                        }
                    }
                    // Wariant B: 2-opt na pozycjach (pv-1, pu-1) — dodaje
                    // (prev(v_id), prev(u)) oraz (v_id, u).
                    {
                        int i = (pv - 1 + m) % m;
                        int j = (pu - 1 + m) % m;
                        if (i > j) std::swap(i, j);
                        if (j - i >= 2 && !(i == 0 && j == m - 1)) {
                            int d = eval_edge(i, j);
                            if (d > best_delta) {
                                best_delta = d;
                                best = {MType::EdgeSwap, i, j, d};
                                found = true;
                            }
                        }
                    }
                }
                else if (state.in_cycle[v_id] && !state.in_cycle[u]
                         && m >= 2 && m < n_total) {
                    // Insert u tuż obok v_id — dwa położenia, każde wprowadza
                    // krawędź (v_id, u) jako jedną z dwóch nowych krawędzi.
                    int pv = state.pos[v_id];
                    int pos_after  = pv;
                    int pos_before = (pv - 1 + m) % m;
                    int d1 = eval_insert(pos_after,  u);
                    if (d1 > best_delta) {
                        best_delta = d1; best = {MType::Insert, pos_after,  u, d1}; found = true;
                    }
                    int d2 = eval_insert(pos_before, u);
                    if (d2 > best_delta) {
                        best_delta = d2; best = {MType::Insert, pos_before, u, d2}; found = true;
                    }
                }
                else if (!state.in_cycle[v_id] && state.in_cycle[u]
                         && m >= 2 && m < n_total) {
                    // Insert v_id tuż obok u — symetryczny przypadek
                    // (lista nearest niesymetryczna, więc trzeba osobno).
                    int pu = state.pos[u];
                    int pos_after  = pu;
                    int pos_before = (pu - 1 + m) % m;
                    int d1 = eval_insert(pos_after,  v_id);
                    if (d1 > best_delta) {
                        best_delta = d1; best = {MType::Insert, pos_after,  v_id, d1}; found = true;
                    }
                    int d2 = eval_insert(pos_before, v_id);
                    if (d2 > best_delta) {
                        best_delta = d2; best = {MType::Insert, pos_before, v_id, d2}; found = true;
                    }
                }
                // oba poza cyklem — pomijamy
            }
        }

        // Remove — kandydacki gdy nowa krawędź (prev, next) jest kandydacka.
        if (m > 3) {
            for (int i = 0; i < m; i++) {
                int p = sol.cycle[(i - 1 + m) % m];
                int q = sol.cycle[(i + 1) % m];
                if (list_contains(nearest[p], q) || list_contains(nearest[q], p)) {
                    int d = eval_remove(i);
                    if (d > best_delta) {
                        best_delta = d;
                        best = {MType::Remove, i, 0, d};
                        found = true;
                    }
                }
            }
        }

        if (!found) break;
        apply_move(sol, state, best, n_total);
    }

    return sol;
}
