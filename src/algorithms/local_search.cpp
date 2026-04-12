#include "local_search.h"
#include <algorithm>
#include <vector>

namespace {

enum class MoveType { Insert, Remove, NodeSwap, EdgeSwap };

// Reprezentacja ruchu. Interpretacja (a, b) zależy od typu:
//   Insert   — a = pozycja w cyklu (nowy wierzchołek trafia za a), b = id wierzchołka
//   Remove   — a = indeks wierzchołka do usunięcia, b = nieużywane
//   NodeSwap — a, b = indeksy dwóch wierzchołków do zamiany (a < b)
//   EdgeSwap — a, b = indeksy końców dwóch krawędzi do wymiany (2-opt: odwracany
//              jest segment [a+1 .. b])
struct Move {
    MoveType type;
    int a;
    int b;
};

// --- Delty funkcji celu -----------------------------------------------------
// Wszystkie delty są wyrażone w konwencji "objective = sign*Σprofit − Σdist"
// i zwracają wartość taką, że f(nowy) − f(stary) = delta. Poprawa ⇔ delta > 0.

int delta_insert(const Instance& inst, const Solution& sol,
                 int pos, int v, const ObjectiveConfig& cfg) {
    int n = sol.size();
    int a = sol.cycle[pos];
    int b = sol.cycle[(pos + 1) % n];
    // Zyskujemy wartość węzła, tracimy przyrost długości krawędzi.
    return cfg.node_value_sign * inst.nodes[v].profit
           - (inst.dist[a][v] + inst.dist[v][b] - inst.dist[a][b]);
}

int delta_remove(const Instance& inst, const Solution& sol,
                 int i, const ObjectiveConfig& cfg) {
    int n = sol.size();
    int v    = sol.cycle[i];
    int prev = sol.cycle[(i - 1 + n) % n];
    int next = sol.cycle[(i + 1) % n];
    return -cfg.node_value_sign * inst.nodes[v].profit
           + inst.dist[prev][v] + inst.dist[v][next]
           - inst.dist[prev][next];
}

int delta_node_swap(const Instance& inst, const Solution& sol, int i, int j) {
    int n = sol.size();
    if (i > j) std::swap(i, j);

    int vi = sol.cycle[i];
    int vj = sol.cycle[j];
    int pi = sol.cycle[(i - 1 + n) % n];
    int ni = sol.cycle[(i + 1) % n];
    int pj = sol.cycle[(j - 1 + n) % n];
    int nj = sol.cycle[(j + 1) % n];

    // Sąsiedztwo bezpośrednie: vi i vj dzielą jedną krawędź, która po zamianie
    // tylko się odwraca (długość bez zmian).
    bool adjacent_forward  = ((i + 1) % n == j); // vi → vj
    bool adjacent_backward = ((j + 1) % n == i); // vj → vi (przez zamknięcie)

    if (adjacent_forward) {
        // Stare: (pi, vi), (vj, nj).  Nowe: (pi, vj), (vi, nj).
        return inst.dist[pi][vi] + inst.dist[vj][nj]
             - inst.dist[pi][vj] - inst.dist[vi][nj];
    }
    if (adjacent_backward) {
        // Stare: (pj, vj), (vi, ni).  Nowe: (pj, vi), (vj, ni).
        return inst.dist[pj][vj] + inst.dist[vi][ni]
             - inst.dist[pj][vi] - inst.dist[vj][ni];
    }
    // Niesąsiednie: cztery krawędzie znikają, cztery nowe się pojawiają.
    return inst.dist[pi][vi] + inst.dist[vi][ni]
         + inst.dist[pj][vj] + inst.dist[vj][nj]
         - inst.dist[pi][vj] - inst.dist[vj][ni]
         - inst.dist[pj][vi] - inst.dist[vi][nj];
}

int delta_edge_swap(const Instance& inst, const Solution& sol, int i, int j) {
    // 2-opt: usuwamy krawędzie (ci, ci+1) i (cj, cj+1),
    // dodajemy (ci, cj) i (ci+1, cj+1). Segment [i+1 .. j] zostaje odwrócony.
    int n = sol.size();
    int ci  = sol.cycle[i];
    int ci1 = sol.cycle[(i + 1) % n];
    int cj  = sol.cycle[j];
    int cj1 = sol.cycle[(j + 1) % n];
    return inst.dist[ci][ci1] + inst.dist[cj][cj1]
         - inst.dist[ci][cj]  - inst.dist[ci1][cj1];
}

int compute_delta(const Instance& inst, const Solution& sol,
                  const Move& m, const ObjectiveConfig& cfg) {
    switch (m.type) {
        case MoveType::Insert:   return delta_insert(inst, sol, m.a, m.b, cfg);
        case MoveType::Remove:   return delta_remove(inst, sol, m.a, cfg);
        case MoveType::NodeSwap: return delta_node_swap(inst, sol, m.a, m.b);
        case MoveType::EdgeSwap: return delta_edge_swap(inst, sol, m.a, m.b);
    }
    return 0;
}

// --- Aplikowanie ruchów -----------------------------------------------------

void apply_move(Solution& sol, const Move& m) {
    switch (m.type) {
        case MoveType::Insert:
            sol.cycle.insert(sol.cycle.begin() + m.a + 1, m.b);
            break;
        case MoveType::Remove:
            sol.cycle.erase(sol.cycle.begin() + m.a);
            break;
        case MoveType::NodeSwap:
            std::swap(sol.cycle[m.a], sol.cycle[m.b]);
            break;
        case MoveType::EdgeSwap:
            std::reverse(sol.cycle.begin() + m.a + 1,
                         sol.cycle.begin() + m.b + 1);
            break;
    }
}

// --- Generowanie pełnej listy ruchów bieżącego sąsiedztwa -------------------

void collect_moves(const Instance& inst, const Solution& sol,
                   LSNeighborhood neigh, std::vector<Move>& out) {
    int n_total = inst.n();
    int m = sol.size();
    out.clear();

    std::vector<bool> in_cycle(n_total, false);
    for (int v : sol.cycle) in_cycle[v] = true;

    // Ruchy inter-route: Insert — każdy wierzchołek spoza cyklu na każdą pozycję.
    if (m < n_total && m >= 2) {
        for (int pos = 0; pos < m; pos++) {
            for (int v = 0; v < n_total; v++) {
                if (in_cycle[v]) continue;
                out.push_back({MoveType::Insert, pos, v});
            }
        }
    }
    // Ruchy inter-route: Remove — usuwamy wierzchołek z cyklu (pilnujemy m ≥ 3).
    if (m > 3) {
        for (int i = 0; i < m; i++) {
            out.push_back({MoveType::Remove, i, 0});
        }
    }
    // Ruchy intra-route. Dla m < 4 każda zamiana/2-opt jest trywialnym no-opem
    // (w cyklu 2- lub 3-elementowym wielozbiór krawędzi się nie zmienia),
    // więc nie generujemy takich pseudo-ruchów — inaczej delta ≠ 0 byłaby
    // błędna i steepest wpadałby w nieskończoną pętlę.
    if (m < 4) return;
    if (neigh == LSNeighborhood::NodeSwap) {
        for (int i = 0; i < m - 1; i++) {
            for (int j = i + 1; j < m; j++) {
                out.push_back({MoveType::NodeSwap, i, j});
            }
        }
    } else {
        // 2-opt: pary (i, j) z j ≥ i+2. Para (0, m-1) dałaby tylko odwrócenie
        // całego cyklu, więc ją pomijamy.
        for (int i = 0; i < m - 1; i++) {
            for (int j = i + 2; j < m; j++) {
                if (i == 0 && j == m - 1) continue;
                out.push_back({MoveType::EdgeSwap, i, j});
            }
        }
    }
}

} // namespace

Solution local_search(const Instance& inst, Solution sol,
                      LSMode mode, LSNeighborhood neigh,
                      std::mt19937& rng,
                      const ObjectiveConfig& config) {
    std::vector<Move> moves;

    while (true) {
        collect_moves(inst, sol, neigh, moves);
        if (moves.empty()) break;

        if (mode == LSMode::Steepest) {
            int best_delta = 0;
            int best_idx   = -1;
            for (int k = 0; k < static_cast<int>(moves.size()); k++) {
                int d = compute_delta(inst, sol, moves[k], config);
                if (d > best_delta) {
                    best_delta = d;
                    best_idx   = k;
                }
            }
            if (best_idx < 0) break; // brak poprawy po przejrzeniu całego M(x)
            apply_move(sol, moves[best_idx]);
        } else {
            // Greedy: randomizujemy kolejność przeglądania i stosujemy
            // pierwszy ruch poprawiający. Koniec gdy pełne przejście nie da
            // poprawy.
            std::shuffle(moves.begin(), moves.end(), rng);
            bool improved = false;
            for (const auto& mv : moves) {
                int d = compute_delta(inst, sol, mv, config);
                if (d > 0) {
                    apply_move(sol, mv);
                    improved = true;
                    break;
                }
            }
            if (!improved) break;
        }
    }

    return sol;
}
