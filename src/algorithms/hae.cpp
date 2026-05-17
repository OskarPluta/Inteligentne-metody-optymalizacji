#include "hae.h"
#include "random_solution.h"
#include "local_search_cm.h"
#include "phase2.h"
#include <algorithm>
#include <chrono>
#include <limits>
#include <set>
#include <unordered_set>
#include <utility>
#include <vector>

namespace {

using EdgeKey = std::pair<int,int>;
using EdgeSet = std::set<EdgeKey>;

EdgeKey edge_key(int a, int b) { return std::minmax(a, b); }

EdgeSet cycle_edges(const std::vector<int>& cyc) {
    EdgeSet es;
    int m = static_cast<int>(cyc.size());
    if (m < 2) return es;
    for (int i = 0; i < m; i++)
        es.insert(edge_key(cyc[i], cyc[(i + 1) % m]));
    return es;
}

// 2-regret insertion do osiągnięcia `target_size` (taki sam jak w LNS).
void repair_regret(Solution& sol, const Instance& inst, int target_size) {
    int n = inst.n();
    std::vector<char> in_cycle(n, 0);
    for (int v : sol.cycle) in_cycle[v] = 1;

    // Jeżeli zostało za mało wierzchołków by liczyć 2-regret, dosadzamy
    // pierwszy najlepszy (najmniejszy koszt wstawienia) przy m ≤ 1.
    while (sol.size() < target_size && sol.size() < n) {
        int m = sol.size();

        if (m == 0) {
            // Wybieramy wierzchołek o najwyższym profit_sign*profit (proste seedowanie).
            int best_v = -1;
            for (int v = 0; v < n; v++) if (!in_cycle[v]) { best_v = v; break; }
            if (best_v == -1) break;
            sol.cycle.push_back(best_v);
            in_cycle[best_v] = 1;
            continue;
        }
        if (m == 1) {
            // Dosadzamy najbliższy wierzchołek żeby uzyskać cykl o 2 wierzchołkach.
            int u = sol.cycle[0];
            int best_v = -1;
            int best_d = std::numeric_limits<int>::max();
            for (int v = 0; v < n; v++) {
                if (in_cycle[v]) continue;
                int d = inst.dist[u][v];
                if (d < best_d) { best_d = d; best_v = v; }
            }
            if (best_v == -1) break;
            sol.cycle.push_back(best_v);
            in_cycle[best_v] = 1;
            continue;
        }

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
        if (best_v == -1) break;

        sol.cycle.insert(sol.cycle.begin() + best_pos + 1, best_v);
        in_cycle[best_v] = 1;
    }
}

// Łączy listę podścieżek losowo: tasujemy kolejność i każdą z prawd. 1/2 odwracamy.
std::vector<int> stitch_subpaths(std::vector<std::vector<int>> sub,
                                 std::mt19937& rng) {
    std::shuffle(sub.begin(), sub.end(), rng);
    std::uniform_int_distribution<int> coin(0, 1);
    std::vector<int> out;
    for (auto& s : sub) {
        if (s.size() >= 2 && coin(rng)) std::reverse(s.begin(), s.end());
        out.insert(out.end(), s.begin(), s.end());
    }
    return out;
}

// Operator 1: zachowuje wspólne wierzchołki i krawędzie obu rodziców.
Solution recombine_op1(const Solution& pa, const Solution& pb,
                       const Instance& inst, std::mt19937& rng) {
    std::unordered_set<int> in_b;
    for (int v : pb.cycle) in_b.insert(v);

    std::vector<int> common_v;
    for (int v : pa.cycle) if (in_b.count(v)) common_v.push_back(v);

    EdgeSet ea = cycle_edges(pa.cycle);
    EdgeSet eb = cycle_edges(pb.cycle);
    EdgeSet common_e;
    for (const auto& e : ea) if (eb.count(e)) common_e.insert(e);

    // Graf wspólnych krawędzi — każdy wierzchołek ma stopień ≤ 2,
    // więc składowe to ścieżki (lub w skrajnym przypadku cykl,
    // jeśli rodzice są identyczni).
    int n = inst.n();
    std::vector<std::vector<int>> adj(n);
    for (const auto& [a, b] : common_e) {
        adj[a].push_back(b);
        adj[b].push_back(a);
    }

    std::vector<std::vector<int>> subpaths;
    std::vector<char> visited(n, 0);
    for (int v : common_v) {
        if (visited[v]) continue;

        // Idź od v aż natrafisz na koniec (deg ≤ 1 → next == -1) lub wrócisz do v.
        int prev = -1, cur = v;
        while (true) {
            int next = -1;
            for (int u : adj[cur]) if (u != prev) { next = u; break; }
            if (next == -1) break;
            if (next == v) break; // cykl — start od v jest OK
            prev = cur;
            cur = next;
        }
        int start = cur;

        // Buduj ścieżkę od `start`, dalej `visited` zapobiega zapętleniu w cyklu.
        std::vector<int> path;
        prev = -1;
        cur = start;
        while (true) {
            path.push_back(cur);
            visited[cur] = 1;
            int next = -1;
            for (int u : adj[cur]) if (u != prev && !visited[u]) { next = u; break; }
            if (next == -1) break;
            prev = cur;
            cur = next;
        }
        subpaths.push_back(std::move(path));
    }

    Solution sol;
    sol.cycle = stitch_subpaths(std::move(subpaths), rng);
    return sol;
}

// Operator 2: rodzic A jako baza; usuwamy wierzchołki i krawędzie spoza B.
Solution recombine_op2(const Solution& pa, const Solution& pb,
                       const Instance& inst, std::mt19937& rng) {
    (void)inst;
    std::unordered_set<int> in_b;
    for (int v : pb.cycle) in_b.insert(v);

    // Krok 1: usuń wierzchołki nieobecne w B (luki zamykają się automatycznie).
    std::vector<int> cyc;
    cyc.reserve(pa.cycle.size());
    for (int v : pa.cycle) if (in_b.count(v)) cyc.push_back(v);
    if (cyc.size() < 2) {
        Solution s; s.cycle = std::move(cyc); return s;
    }

    EdgeSet eb = cycle_edges(pb.cycle);

    // Krok 2: znajdź, które krawędzie cyklu są w B; reszta zostanie usunięta.
    int m = static_cast<int>(cyc.size());
    std::vector<char> keep_edge(m); // krawędź (cyc[i], cyc[(i+1)%m])
    int cut = -1;
    for (int i = 0; i < m; i++) {
        keep_edge[i] = eb.count(edge_key(cyc[i], cyc[(i + 1) % m])) > 0;
        if (!keep_edge[i] && cut == -1) cut = i;
    }

    std::vector<std::vector<int>> subpaths;
    if (cut == -1) {
        // Wszystkie krawędzie zachowane (rodzice mają wspólny cykl na wspólnych wierzchołkach).
        subpaths.push_back(cyc);
    } else {
        int start = (cut + 1) % m;
        std::vector<int> cur;
        for (int k = 0; k < m; k++) {
            int idx = (start + k) % m;
            cur.push_back(cyc[idx]);
            if (!keep_edge[idx]) {
                subpaths.push_back(std::move(cur));
                cur.clear();
            }
        }
        if (!cur.empty()) subpaths.push_back(std::move(cur));
    }

    // Krok 3: usuń podścieżki długości 1 — to wierzchołki, które straciły
    // obie sąsiednie krawędzie (zgodnie ze specyfikacją).
    std::vector<std::vector<int>> filtered;
    filtered.reserve(subpaths.size());
    for (auto& s : subpaths) if (s.size() >= 2) filtered.push_back(std::move(s));

    Solution sol;
    sol.cycle = stitch_subpaths(std::move(filtered), rng);
    return sol;
}

// Operator 3: rodzic A jako baza; usuwamy tylko wierzchołki nieobecne w B.
Solution recombine_op3(const Solution& pa, const Solution& pb,
                       const Instance& /*inst*/, std::mt19937& /*rng*/) {
    std::unordered_set<int> in_b;
    for (int v : pb.cycle) in_b.insert(v);
    Solution sol;
    sol.cycle.reserve(pa.cycle.size());
    for (int v : pa.cycle) if (in_b.count(v)) sol.cycle.push_back(v);
    return sol;
}

Solution recombine(HaeOperator op, const Solution& pa, const Solution& pb,
                   const Instance& inst, std::mt19937& rng) {
    switch (op) {
        case HaeOperator::Op1: return recombine_op1(pa, pb, inst, rng);
        case HaeOperator::Op2: return recombine_op2(pa, pb, inst, rng);
        case HaeOperator::Op3: return recombine_op3(pa, pb, inst, rng);
    }
    return recombine_op1(pa, pb, inst, rng);
}

} // namespace

Solution hae(const Instance& inst,
             const std::vector<std::vector<int>>& nearest,
             std::mt19937& rng,
             double time_budget_s,
             int& iters_out,
             HaeOperator op,
             bool use_local_search,
             int pop_size,
             const ObjectiveConfig& config) {
    auto t0 = std::chrono::steady_clock::now();
    auto elapsed = [&]() {
        return std::chrono::duration<double>(
            std::chrono::steady_clock::now() - t0).count();
    };

    // Populacja początkowa: pop_size różnych (po wartości funkcji celu) rozwiązań po LP.
    std::vector<Solution>   pop;
    std::vector<int>        obj;
    std::unordered_set<int> obj_set;
    pop.reserve(pop_size);
    obj.reserve(pop_size);

    const int init_attempt_cap = pop_size * 50;
    for (int attempt = 0;
         static_cast<int>(pop.size()) < pop_size && attempt < init_attempt_cap;
         attempt++) {
        if (elapsed() >= time_budget_s) break;
        Solution s = local_search_cm(inst, random_solution(inst, rng), nearest, config);
        int o = s.objective(inst, config);
        if (obj_set.insert(o).second) {
            pop.push_back(std::move(s));
            obj.push_back(o);
        }
    }
    if (pop.size() < 2) {
        iters_out = 0;
        return pop.empty() ? Solution{} : pop.front();
    }

    auto worst_index = [&]() {
        int idx = 0;
        for (int i = 1; i < static_cast<int>(obj.size()); i++)
            if (obj[i] < obj[idx]) idx = i;
        return idx;
    };
    auto best_index = [&]() {
        int idx = 0;
        for (int i = 1; i < static_cast<int>(obj.size()); i++)
            if (obj[i] > obj[idx]) idx = i;
        return idx;
    };

    int iters = 0;
    while (elapsed() < time_budget_s) {
        int m = static_cast<int>(pop.size());
        std::uniform_int_distribution<int> pick(0, m - 1);
        int a = pick(rng);
        int b = pick(rng);
        while (b == a) b = pick(rng);

        int target_size = pop[a].size();

        Solution y = recombine(op, pop[a], pop[b], inst, rng);
        repair_regret(y, inst, target_size);
        y = phase2_remove(y, inst, config);
        if (use_local_search) {
            y = local_search_cm(inst, y, nearest, config);
        }

        int y_obj = y.objective(inst, config);
        int w = worst_index();
        if (y_obj > obj[w] && obj_set.find(y_obj) == obj_set.end()) {
            obj_set.erase(obj[w]);
            obj_set.insert(y_obj);
            pop[w] = std::move(y);
            obj[w] = y_obj;
        }
        iters++;
    }

    iters_out = iters;
    return pop[best_index()];
}
