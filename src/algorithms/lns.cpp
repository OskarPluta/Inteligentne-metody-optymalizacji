#include "lns.h"
#include "random_solution.h"
#include "local_search_cm.h"
#include <algorithm>
#include <chrono>
#include <limits>
#include <utility>
#include <vector>

namespace {

// Destroy: usuwa `count` wierzchołków z cyklu, losując ich pozycje
// proporcjonalnie do "kosztu" ich obecności w rozwiązaniu.
//   waga(v) = dist[prev][v] + dist[v][next] − node_value_sign·profit[v]
// Im większa waga, tym chętniej v jest usuwany — to są wierzchołki
// z długimi sąsiednimi krawędziami i/lub niskim zyskiem.
void destroy(Solution& sol, const Instance& inst, std::mt19937& rng,
             int count, const ObjectiveConfig& config) {
    for (int k = 0; k < count; k++) {
        int m = sol.size();
        if (m <= 3) break;

        std::vector<double> w(m);
        double w_min = std::numeric_limits<double>::infinity();
        for (int i = 0; i < m; i++) {
            int v    = sol.cycle[i];
            int prev = sol.cycle[(i - 1 + m) % m];
            int next = sol.cycle[(i + 1) % m];
            double wi = static_cast<double>(inst.dist[prev][v])
                      + static_cast<double>(inst.dist[v][next])
                      - static_cast<double>(config.node_value_sign) * inst.nodes[v].profit;
            w[i] = wi;
            if (wi < w_min) w_min = wi;
        }
        // Przesuwamy żeby wszystkie wagi były dodatnie (zachowując proporcje).
        double shift = (w_min < 1.0) ? (1.0 - w_min) : 0.0;
        for (auto& x : w) x += shift;

        std::discrete_distribution<int> dist(w.begin(), w.end());
        int idx = dist(rng);
        sol.cycle.erase(sol.cycle.begin() + idx);
    }
}

// Repair: 2-regret insertion. Wstawia wierzchołki spoza cyklu w pozycje
// minimalizujące przyrost długości; wybiera wierzchołek o największym
// 2-żalu (drugie najlepsze miejsce – pierwsze najlepsze). Powtarza
// do osiągnięcia `target_size`.
//
// Tożsame z `regret_phase1` (regret.cpp), ale działa na częściowym
// rozwiązaniu i kończy gdy |cykl| == target_size, a nie n.
void repair_regret(Solution& sol, const Instance& inst, int target_size) {
    int n = inst.n();
    std::vector<char> in_cycle(n, 0);
    for (int v : sol.cycle) in_cycle[v] = 1;

    while (sol.size() < target_size) {
        int m = sol.size();
        int best_v      = -1;
        int best_pos    = -1;
        int best_regret = std::numeric_limits<int>::lowest();

        for (int v = 0; v < n; v++) {
            if (in_cycle[v]) continue;

            int d1 = std::numeric_limits<int>::max();
            int d2 = std::numeric_limits<int>::max();
            int pos1 = -1;

            for (int i = 0; i < m; i++) {
                int a = sol.cycle[i];
                int b = sol.cycle[(i + 1) % m];
                int d = inst.dist[a][v] + inst.dist[v][b] - inst.dist[a][b];
                if (d < d1)      { d2 = d1; d1 = d; pos1 = i; }
                else if (d < d2) { d2 = d; }
            }

            int regret = d2 - d1;
            if (regret > best_regret) {
                best_regret = regret;
                best_v      = v;
                best_pos    = pos1;
            }
        }

        sol.cycle.insert(sol.cycle.begin() + best_pos + 1, best_v);
        in_cycle[best_v] = 1;
    }
}

} // namespace

Solution lns(const Instance& inst,
             const std::vector<std::vector<int>>& nearest,
             std::mt19937& rng,
             double time_budget_s,
             int& iters_out,
             bool use_local_search,
             double destroy_fraction,
             const ObjectiveConfig& config) {
    Solution x = local_search_cm(inst, random_solution(inst, rng), nearest, config);
    int x_obj = x.objective(inst, config);

    Solution best = x;
    int best_obj = x_obj;

    int iters = 0;
    auto t0 = std::chrono::steady_clock::now();
    while (true) {
        double elapsed = std::chrono::duration<double>(
            std::chrono::steady_clock::now() - t0).count();
        if (elapsed >= time_budget_s) break;

        Solution y = x;
        int target_size = y.size();
        int to_remove   = std::max(1, static_cast<int>(destroy_fraction * target_size));

        destroy(y, inst, rng, to_remove, config);
        repair_regret(y, inst, target_size);
        if (use_local_search) {
            y = local_search_cm(inst, y, nearest, config);
        }

        int y_obj = y.objective(inst, config);
        if (y_obj > x_obj) {
            x     = y;
            x_obj = y_obj;
        }
        if (y_obj > best_obj) {
            best     = std::move(y);
            best_obj = y_obj;
        }
        iters++;
    }

    iters_out = iters;
    return best;
}
