#include "gc.h"
#include "phase2.h"
#include <limits>
#include <vector>

Solution gc_phase1(const Instance& inst, int start, bool use_profit,
                   const ObjectiveConfig& config) {
    int n = inst.n();
    std::vector<bool> visited(n, false);

    // Krok 1: znajdź wierzchołek najbliższy startowi i zbuduj 2-elementowy cykl
    int nearest      = -1;
    int nearest_dist = std::numeric_limits<int>::max();
    for (int v = 0; v < n; v++) {
        if (v == start) continue;
        if (inst.dist[start][v] < nearest_dist) {
            nearest_dist = inst.dist[start][v];
            nearest      = v;
        }
    }

    Solution sol;
    sol.cycle = {start, nearest};
    visited[start]   = true;
    visited[nearest] = true;

    // Krok 2: wstawiaj kolejne wierzchołki aż cykl obejmie wszystkie
    while (static_cast<int>(sol.cycle.size()) < n) {
        int  best_v      = -1;
        int  best_pos    = -1;           // indeks: wstawiamy v między cycle[best_pos] a cycle[best_pos+1]
        int  best_score  = std::numeric_limits<int>::lowest();

        int m = static_cast<int>(sol.cycle.size());

        for (int v = 0; v < n; v++) {
            if (visited[v]) continue;

            // Znajdź najlepszą pozycję wstawienia v do bieżącego cyklu
            int best_delta_dist = std::numeric_limits<int>::max();
            int best_pos_for_v  = -1;

            for (int i = 0; i < m; i++) {
                int a = sol.cycle[i];
                int b = sol.cycle[(i + 1) % m];

                // Wzrost długości cyklu po wstawieniu v między a i b:
                //   delta_dist = dist[a][v] + dist[v][b] − dist[a][b]
                int delta_dist = inst.dist[a][v] + inst.dist[v][b] - inst.dist[a][b];

                if (delta_dist < best_delta_dist) {
                    best_delta_dist = delta_dist;
                    best_pos_for_v  = i;
                }
            }

            // Ocena wstawienia wierzchołka v (w jego najlepsze miejsce):
            // GCa: minimalizuj delta_dist → score = −delta_dist
            // GC:  maksymalizuj zysk−delta → score = node_value_sign*profit[v] − delta_dist
            int score = use_profit
                ? config.node_value_sign * inst.nodes[v].profit - best_delta_dist
                : -best_delta_dist;

            if (score > best_score) {
                best_score = score;
                best_v     = v;
                best_pos   = best_pos_for_v;
            }
        }

        // Wstaw best_v po indeksie best_pos
        sol.cycle.insert(sol.cycle.begin() + best_pos + 1, best_v);
        visited[best_v] = true;
    }

    return sol;
}

/*
PSEUDOKOD GC FAZA I:
n := liczba wierzchołków
visited[0..n-1] := false
nearest := wierzchołek najbliższy do start
cykl := [start, nearest]
visited[start] := true
visited[nearest] := true

dopóki |cykl| < n:
    best_score := -∞

    dla każdego nieodwiedzonego v:
        best_Δdist := +∞

        dla każdej krawędzi (a, b) w cyklu:
            Δdist := dist[a][v] + dist[v][b] - dist[a][b]
            jeśli Δdist < best_Δdist:
                best_Δdist := Δdist
                best_pos_v := pozycja między a i b

        jeśli use_profit:
            score := sign * profit[v] - best_Δdist
        wpp:
            score := -best_Δdist

        jeśli score > best_score:
            best_score := score
            best_v := v
            best_pos := best_pos_v

    wstaw best_v do cyklu po pozycji best_pos

PSEUDOKOD GC:
cykl := GC_FAZA_I(start, use_profit)
cykl := FAZA_II(cykl)
*/

Solution greedy_cycle(const Instance& inst, int start, bool use_profit,
                      const ObjectiveConfig& config) {
    Solution phase1 = gc_phase1(inst, start, use_profit, config);
    return phase2_remove(phase1, inst, config);
}
