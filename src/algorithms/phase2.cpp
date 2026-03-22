#include "phase2.h"

Solution phase2_remove(const Solution& phase1_sol, const Instance& inst,
                        const ObjectiveConfig& config) {
    Solution sol = phase1_sol;

    while (sol.size() > 2) {
        int n = sol.size();
        int best_idx   = -1;
        int best_delta = 0; // usuwamy tylko gdy delta > 0 (poprawa)

        for (int i = 0; i < n; i++) {
            int v    = sol.cycle[i];
            int prev = sol.cycle[(i - 1 + n) % n];
            int next = sol.cycle[(i + 1) % n];

            // Usunięcie v zastępuje krawędzie (prev→v)+(v→next) krawędzią (prev→next).
            // Zmiana funkcji celu po usunięciu v:
            //   delta = −node_value_sign*profit[v]          (tracimy wartość węzła)
            //         + dist[prev][v] + dist[v][next]       (zyskujemy oszczędność na trasie)
            //         − dist[prev][next]                    (płacimy za nową krawędź)
            int delta = -config.node_value_sign * inst.nodes[v].profit
                        + inst.dist[prev][v]
                        + inst.dist[v][next]
                        - inst.dist[prev][next];
            


            if (delta > best_delta) {
                best_delta = delta;
                best_idx   = i;
            }
        }

        if (best_idx == -1) break; // brak poprawy — kończymy

        sol.cycle.erase(sol.cycle.begin() + best_idx);
    }

    return sol;
}

/*
PSEUDOKOD FAZA II:
dopóki |cykl| > 2:
    best_delta := 0

    dla każdego wierzchołka v w cyklu:
        prev := sąsiad v z lewej
        next := sąsiad v z prawej
        δ := -sign * profit[v] + dist[prev][v] + dist[v][next] - dist[prev][next]

        jeśli δ > best_delta:
            best_delta := δ
            best_idx := indeks v

    jeśli best_delta = 0:
        przerwij (brak poprawy)

    usuń cykl[best_idx]
*/
