#pragma once
#include "types.hpp"
#include "phase2.hpp"

// Nearest Neighbor - Phase I
// use_profit: false = NNa (distance only), true = NN (distance - profit)
inline Solution nearest_neighbor(const Instance& inst, int start, bool use_profit) {
    int n = inst.n;
    std::vector<bool> visited(n, false);
    Solution sol;
    
    sol.cycle.push_back(start);
    visited[start] = true;
    
    for (int step = 1; step < n; step++) {
        int last = sol.cycle.back();
        int best = -1;
        int best_cost = std::numeric_limits<int>::max();
        
        for (int v = 0; v < n; v++) {
            if (visited[v]) continue;
            int cost;
            if (use_profit) {
                cost = inst.dist[last][v] - inst.nodes[v].profit;
            } else {
                cost = inst.dist[last][v];
            }
            if (cost < best_cost) {
                best_cost = cost;
                best = v;
            }
        }
        
        sol.cycle.push_back(best);
        visited[best] = true;
    }
    
    // Phase I complete - all vertices in cycle
    sol.compute_objective(inst);
    return sol;
}

// Full NN with phase II
inline Solution nn_full(const Instance& inst, int start, bool use_profit) {
    Solution sol = nearest_neighbor(inst, start, use_profit);
    phase2_removal(sol, inst);
    return sol;
}
