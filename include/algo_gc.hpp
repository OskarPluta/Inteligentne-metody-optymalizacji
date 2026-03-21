#pragma once
#include "types.hpp"
#include "phase2.hpp"

// Cost of inserting vertex v between cycle[pos] and cycle[(pos+1) % sz]
// = dist(i,v) + dist(v,j) - dist(i,j)
inline int insertion_cost_dist(const std::vector<int>& cycle, int pos, int v, const Instance& inst) {
    int sz = (int)cycle.size();
    int i = cycle[pos];
    int j = cycle[(pos + 1) % sz];
    return inst.dist[i][v] + inst.dist[v][j] - inst.dist[i][j];
}

// Insertion cost with profit: dist_cost - profit(v)
inline int insertion_cost_profit(const std::vector<int>& cycle, int pos, int v, const Instance& inst) {
    return insertion_cost_dist(cycle, pos, v, inst) - inst.nodes[v].profit;
}

// Find best insertion position for vertex v in cycle
// Returns {best_position, best_cost}
// use_profit: if true, cost includes -profit(v)
inline std::pair<int, int> find_best_insertion(const std::vector<int>& cycle, int v,
                                                const Instance& inst, bool use_profit) {
    int sz = (int)cycle.size();
    int best_pos = 0;
    int best_cost = std::numeric_limits<int>::max();
    
    for (int pos = 0; pos < sz; pos++) {
        int cost = use_profit ? insertion_cost_profit(cycle, pos, v, inst)
                              : insertion_cost_dist(cycle, pos, v, inst);
        if (cost < best_cost) {
            best_cost = cost;
            best_pos = pos;
        }
    }
    return {best_pos, best_cost};
}

// Find second best insertion cost for vertex v
inline int find_second_best_cost(const std::vector<int>& cycle, int v, int best_pos,
                                  const Instance& inst, bool use_profit) {
    int sz = (int)cycle.size();
    int second_best = std::numeric_limits<int>::max();
    for (int pos = 0; pos < sz; pos++) {
        if (pos == best_pos) continue;
        int cost = use_profit ? insertion_cost_profit(cycle, pos, v, inst)
                              : insertion_cost_dist(cycle, pos, v, inst);
        if (cost < second_best) {
            second_best = cost;
        }
    }
    return second_best;
}

// Insert vertex v after position pos in cycle
inline void insert_into_cycle(std::vector<int>& cycle, int pos, int v) {
    cycle.insert(cycle.begin() + pos + 1, v);
}

// ======================== Greedy Cycle - Phase I ========================

enum class GCVariant {
    GC_NO_PROFIT,     // GCa - distance only
    GC_WITH_PROFIT,   // GC  - distance - profit
    REGRET_2,         // 2-regret
    WEIGHTED_REGRET_2 // weighted 2-regret
};

// Generic greedy cycle builder (phase I)
// Handles all variants: GCa, GC, 2-regret, weighted 2-regret
inline Solution greedy_cycle_phase1(const Instance& inst, int start, GCVariant variant,
                                     double w_regret = 1.0, double w_greedy = -1.0) {
    int n = inst.n;
    bool use_profit = (variant != GCVariant::GC_NO_PROFIT);
    
    std::vector<bool> in_cycle(n, false);
    Solution sol;
    
    // Start with one vertex
    sol.cycle.push_back(start);
    in_cycle[start] = true;
    
    // Find nearest vertex to start and create initial 2-vertex cycle
    int nearest = -1;
    int nearest_cost = std::numeric_limits<int>::max();
    for (int v = 0; v < n; v++) {
        if (v == start) continue;
        int cost = use_profit ? (inst.dist[start][v] - inst.nodes[v].profit)
                              : inst.dist[start][v];
        if (cost < nearest_cost) {
            nearest_cost = cost;
            nearest = v;
        }
    }
    sol.cycle.push_back(nearest);
    in_cycle[nearest] = true;
    
    // Iteratively add remaining vertices
    while ((int)sol.cycle.size() < n) {
        int best_v = -1;
        int best_v_pos = -1;
        double best_score = -std::numeric_limits<double>::max();
        
        for (int v = 0; v < n; v++) {
            if (in_cycle[v]) continue;
            
            auto [b_pos, b_cost] = find_best_insertion(sol.cycle, v, inst, use_profit);
            
            double score = -std::numeric_limits<double>::max();
            switch (variant) {
                case GCVariant::GC_NO_PROFIT:
                case GCVariant::GC_WITH_PROFIT:
                    // Greedy: pick vertex with smallest insertion cost
                    score = -b_cost; // negate because we pick max score
                    break;
                    
                case GCVariant::REGRET_2: {
                    // 2-regret: pick vertex with largest regret
                    if ((int)sol.cycle.size() < 2) {
                        score = -b_cost;
                    } else {
                        int s_cost = find_second_best_cost(sol.cycle, v, b_pos, inst, use_profit);
                        score = (double)(s_cost - b_cost); // regret
                    }
                    break;
                }
                    
                case GCVariant::WEIGHTED_REGRET_2: {
                    // Weighted: w_regret * regret + w_greedy * best_cost
                    if ((int)sol.cycle.size() < 2) {
                        score = w_greedy * b_cost;
                    } else {
                        int s_cost = find_second_best_cost(sol.cycle, v, b_pos, inst, use_profit);
                        int regret = s_cost - b_cost;
                        score = w_regret * regret + w_greedy * b_cost;
                    }
                    break;
                }
            }
            
            if (score > best_score) {
                best_score = score;
                best_v = v;
                best_v_pos = b_pos;
            }
        }
        
        insert_into_cycle(sol.cycle, best_v_pos, best_v);
        in_cycle[best_v] = true;
    }
    
    sol.compute_objective(inst);
    return sol;
}

// ======================== Full algorithms with Phase II ========================

inline Solution gc_full(const Instance& inst, int start, bool use_profit) {
    GCVariant var = use_profit ? GCVariant::GC_WITH_PROFIT : GCVariant::GC_NO_PROFIT;
    Solution sol = greedy_cycle_phase1(inst, start, var);
    phase2_removal(sol, inst);
    return sol;
}

inline Solution regret2_full(const Instance& inst, int start, bool /*use_profit*/) {
    // Regret always uses profit-aware insertion costs
    Solution sol = greedy_cycle_phase1(inst, start, GCVariant::REGRET_2);
    phase2_removal(sol, inst);
    return sol;
}

inline Solution weighted_regret2_full(const Instance& inst, int start,
                                       double w_regret = 1.0, double w_greedy = -1.0) {
    Solution sol = greedy_cycle_phase1(inst, start, GCVariant::WEIGHTED_REGRET_2, w_regret, w_greedy);
    phase2_removal(sol, inst);
    return sol;
}
