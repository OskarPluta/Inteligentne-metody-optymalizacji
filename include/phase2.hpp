#pragma once
#include "types.hpp"

// Delta of removing vertex at position pos from cycle.
// Removing v (between prev and next) replaces edges (prev,v)+(v,next) with (prev,next).
// We gain: dist(prev,v) + dist(v,next) - dist(prev,next) - profit(v)
// If delta > 0, removal improves objective.
inline int removal_delta(const std::vector<int>& cycle, int pos, const Instance& inst) {
    int sz = (int)cycle.size();
    if (sz <= 2) return std::numeric_limits<int>::min(); // can't remove from tiny cycle
    int v = cycle[pos];
    int prev = cycle[(pos - 1 + sz) % sz];
    int next = cycle[(pos + 1) % sz];
    return inst.dist[prev][v] + inst.dist[v][next] - inst.dist[prev][next] - inst.nodes[v].profit;
}

// Phase II: iteratively remove vertices that improve objective
inline void phase2_removal(Solution& sol, const Instance& inst) {
    bool improved = true;
    while (improved && sol.cycle.size() > 3) {
        improved = false;
        int best_delta = 0;
        int best_pos = -1;
        int sz = (int)sol.cycle.size();
        for (int i = 0; i < sz; i++) {
            int d = removal_delta(sol.cycle, i, inst);
            if (d > best_delta) {
                best_delta = d;
                best_pos = i;
            }
        }
        if (best_pos >= 0) {
            sol.cycle.erase(sol.cycle.begin() + best_pos);
            improved = true;
        }
    }
    sol.compute_objective(inst);
}
