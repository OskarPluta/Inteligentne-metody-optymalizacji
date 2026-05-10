#include "ils.h"
#include "random_solution.h"
#include "local_search_cm.h"
#include <algorithm>
#include <chrono>
#include <utility>

namespace {

// Mała perturbacja: złożenie K losowych ruchów.
// Cel — wyrwać się z lokalnego optimum, ale nie zniszczyć struktury.
void perturb(Solution& sol, const Instance& inst, std::mt19937& rng) {
    const int K = 4;

    int n = inst.n();
    int m = sol.size();
    if (m < 4) return;

    // Lista wierzchołków spoza cyklu — odświeżana po każdej zamianie.
    std::vector<char> in_cycle(n, 0);
    for (int v : sol.cycle) in_cycle[v] = 1;
    std::vector<int> outside;
    outside.reserve(n - m);
    for (int v = 0; v < n; v++) if (!in_cycle[v]) outside.push_back(v);

    std::uniform_int_distribution<int> coin(0, 1);

    for (int k = 0; k < K; k++) {
        int op = coin(rng);

        if (op == 0 && !outside.empty()) {
            // Zamiana: in-cycle ↔ outside (zachowuje |cykl|).
            int i = std::uniform_int_distribution<int>(0, m - 1)(rng);
            int j = std::uniform_int_distribution<int>(0, (int)outside.size() - 1)(rng);
            std::swap(sol.cycle[i], outside[j]);
        } else {
            // Losowy 2-opt: odwracamy segment [i, j].
            int i = std::uniform_int_distribution<int>(0, m - 2)(rng);
            int j = std::uniform_int_distribution<int>(i + 1, m - 1)(rng);
            std::reverse(sol.cycle.begin() + i, sol.cycle.begin() + j + 1);
        }
    }
}

} // namespace

Solution ils(const Instance& inst,
             const std::vector<std::vector<int>>& nearest,
             std::mt19937& rng,
             double time_budget_s,
             int& iters_out,
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
        perturb(y, inst, rng);
        y = local_search_cm(inst, y, nearest, config);
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
