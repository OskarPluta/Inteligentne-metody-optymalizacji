#include "nn.h"
#include "phase2.h"
#include <limits>
#include <vector>

Solution nn_phase1(const Instance& inst, int start, bool use_profit,
                   const ObjectiveConfig& config) {
    int n = inst.n();
    std::vector<bool> visited(n, false);

    Solution sol;
    sol.cycle.push_back(start);
    visited[start] = true;

    for (int step = 1; step < n; step++) {
        int last      = sol.cycle.back();
        int best      = -1;
        int best_score = std::numeric_limits<int>::lowest();

        for (int v = 0; v < n; v++) {
            if (visited[v]) continue;

            // NNa: minimalizujemy odległość  → score = −dist
            // NN:  maksymalizujemy zysk−dist  → score = node_value_sign*profit − dist
            int score = use_profit
                ? config.node_value_sign * inst.nodes[v].profit - inst.dist[last][v]
                : -inst.dist[last][v];

            if (score > best_score) {
                best_score = score;
                best       = v;
            }
        }

        sol.cycle.push_back(best);
        visited[best] = true;
    }

    return sol; // cykl zamknięty niejawnie (cycle[last] → cycle[0] obsługuje objective())
}

Solution nearest_neighbor(const Instance& inst, int start, bool use_profit,
                           const ObjectiveConfig& config) {
    Solution phase1 = nn_phase1(inst, start, use_profit, config);
    return phase2_remove(phase1, inst, config);
}

/*
PSEUDOKOD NN FAZA I:
n := liczba wierzchołków
visited[0..n-1] := false
cykl := [start]
visited[start] := true
dla step od 1 do n-1:
    last := ostatni wierzchołek w cyklu
    best_score := -∞

    dla każdego nieodwiedzonego v:
        jeśli use_profit:
            score := sign * profit[v] - dist[last][v]
        wpp:
            score := -dist[last][v]

        jeśli score > best_score:
            best_score := score
            best := v

    dołącz best do cyklu
    visited[best] := true

zamknij cykl krawędzią z ostatniego do pierwszego

PSEUDOKOD NN:
cykl := NN_FAZA_I(start, use_profit)
cykl := FAZA_II(cykl)
*/