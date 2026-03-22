#include "regret.h"
#include "phase2.h"
#include <limits>
#include <vector>

Solution regret_phase1(const Instance& inst, int start,
                       const ObjectiveConfig& config) {
    // config nie jest używany w fazie I — zysk odpada z obliczeń żalu
    // (profit[v] jest stały dla danego v, więc anuluje się przy odejmowaniu).
    // config jest używany w fazie II (phase2_remove).
    (void)config;

    int n = inst.n();
    std::vector<bool> visited(n, false);

    // Inicjalizacja: identyczna jak GC — start + najbliższy → 2-elementowy cykl
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

    while (static_cast<int>(sol.cycle.size()) < n) {
        int m = static_cast<int>(sol.cycle.size());

        int best_v      = -1;
        int best_pos    = -1;
        int best_regret = std::numeric_limits<int>::lowest();

        for (int v = 0; v < n; v++) {
            if (visited[v]) continue;

            // Dla każdej pozycji wstawienia oblicz delta_dist
            int delta_dist_1st  = std::numeric_limits<int>::max(); // najlepsza (najmniejsza)
            int delta_dist_2nd  = std::numeric_limits<int>::max(); // druga najlepsza
            int best_pos_for_v  = -1;

            for (int i = 0; i < m; i++) {
                int a = sol.cycle[i];
                int b = sol.cycle[(i + 1) % m];

                // Wzrost długości cyklu po wstawieniu v między a i b
                int delta_dist = inst.dist[a][v] + inst.dist[v][b] - inst.dist[a][b];

                if (delta_dist < delta_dist_1st) {
                    delta_dist_2nd = delta_dist_1st; // poprzednia najlepsza staje się drugą
                    delta_dist_1st = delta_dist;
                    best_pos_for_v = i;
                } else if (delta_dist < delta_dist_2nd) {
                    delta_dist_2nd = delta_dist;
                }
            }

            // 2-regret(v) = różnica między drugą a pierwszą opcją
            // Gdy cykl ma tylko 2 pozycje i obie są symetryczne, regret = 0
            int regret = delta_dist_2nd - delta_dist_1st;

            if (regret > best_regret) {
                best_regret = regret;
                best_v      = v;
                best_pos    = best_pos_for_v;
            }
        }

        sol.cycle.insert(sol.cycle.begin() + best_pos + 1, best_v);
        visited[best_v] = true;
    }

    return sol;
}

/*
PSEUDOKOD 2-REGRET FAZA I:
n := liczba wierzchołków
visited[0..n-1] := false
nearest := wierzchołek najbliższy do start
cykl := [start, nearest]
visited[start] := true
visited[nearest] := true

dopóki |cykl| < n:
    best_regret := -∞

    dla każdego nieodwiedzonego v:
        Δ₁ := +∞   (najlepsza delta)
        Δ₂ := +∞   (druga najlepsza delta)

        dla każdej krawędzi (a, b) w cyklu:
            Δ := dist[a][v] + dist[v][b] - dist[a][b]
            jeśli Δ < Δ₁:
                Δ₂ := Δ₁
                Δ₁ := Δ
                best_pos_v := pozycja między a i b
            wpp jeśli Δ < Δ₂:
                Δ₂ := Δ

        żal := Δ₂ - Δ₁

        jeśli żal > best_regret:
            best_regret := żal
            best_v := v
            best_pos := best_pos_v

    wstaw best_v do cyklu po pozycji best_pos

(profit nie pojawia się — dla danego v jest stały, więc skraca się przy Δ₂ - Δ₁)

PSEUDOKOD 2-REGRET:
cykl := REGRET_FAZA_I(start)
cykl := FAZA_II(cykl)
*/

Solution regret2(const Instance& inst, int start,
                 const ObjectiveConfig& config) {
    Solution phase1 = regret_phase1(inst, start, config);
    return phase2_remove(phase1, inst, config);
}
