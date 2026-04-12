#include "random_walk.h"
#include <algorithm>
#include <chrono>
#include <vector>

namespace {

// Wykonuje losowo wybrany ruch z pełnego sąsiedztwa (oba typy intra + inter).
// Modyfikuje `sol` w miejscu. Wybiera typ ruchu z rozkładu jednostajnego pośród
// typów dostępnych przy bieżącym rozmiarze cyklu.
void apply_random_move(const Instance& inst, Solution& sol, std::mt19937& rng) {
    int n_total = inst.n();
    int m = sol.size();

    std::vector<int> types;
    if (m >= 2 && m < n_total) types.push_back(0);   // Insert
    if (m > 3)                  types.push_back(1);   // Remove
    if (m >= 4) {
        types.push_back(2);                           // NodeSwap
        types.push_back(3);                           // EdgeSwap
    }
    if (types.empty()) return;

    int t = types[std::uniform_int_distribution<int>(0, (int)types.size() - 1)(rng)];

    if (t == 0) {
        // Insert: losowa pozycja w cyklu, losowy wierzchołek spoza cyklu.
        std::vector<bool> in_cycle(n_total, false);
        for (int v : sol.cycle) in_cycle[v] = true;
        std::vector<int> outside;
        outside.reserve(n_total - m);
        for (int v = 0; v < n_total; v++) if (!in_cycle[v]) outside.push_back(v);
        int pos = std::uniform_int_distribution<int>(0, m - 1)(rng);
        int v   = outside[std::uniform_int_distribution<int>(0, (int)outside.size() - 1)(rng)];
        sol.cycle.insert(sol.cycle.begin() + pos + 1, v);
    } else if (t == 1) {
        int i = std::uniform_int_distribution<int>(0, m - 1)(rng);
        sol.cycle.erase(sol.cycle.begin() + i);
    } else if (t == 2) {
        int i = std::uniform_int_distribution<int>(0, m - 1)(rng);
        int j = std::uniform_int_distribution<int>(0, m - 1)(rng);
        while (j == i) j = std::uniform_int_distribution<int>(0, m - 1)(rng);
        std::swap(sol.cycle[i], sol.cycle[j]);
    } else {
        // EdgeSwap (2-opt) — losowa para (i, j) z j ≥ i+2, (i,j) ≠ (0,m-1).
        int i, j;
        do {
            i = std::uniform_int_distribution<int>(0, m - 3)(rng);
            j = std::uniform_int_distribution<int>(i + 2, m - 1)(rng);
        } while (i == 0 && j == m - 1);
        std::reverse(sol.cycle.begin() + i + 1, sol.cycle.begin() + j + 1);
    }
}

} // namespace

Solution random_walk(const Instance& inst, Solution init,
                     double time_budget_s,
                     std::mt19937& rng,
                     const ObjectiveConfig& config) {
    Solution best = init;
    int best_obj  = best.objective(inst, config);

    Solution cur = init;
    auto t0 = std::chrono::steady_clock::now();

    // Sprawdzamy czas co `check_every` iteracji, by ograniczyć narzut chrono.
    constexpr int check_every = 64;
    int iter = 0;
    while (true) {
        apply_random_move(inst, cur, rng);
        int obj = cur.objective(inst, config);
        if (obj > best_obj) {
            best_obj = obj;
            best     = cur;
        }
        if (++iter % check_every == 0) {
            auto t1 = std::chrono::steady_clock::now();
            if (std::chrono::duration<double>(t1 - t0).count() >= time_budget_s) break;
        }
    }
    return best;
}
