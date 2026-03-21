#pragma once
#include "types.hpp"
#include "algo_random.hpp"
#include "algo_nn.hpp"
#include "algo_gc.hpp"

struct ExperimentResult {
    std::string algo_name;
    std::string instance_name;
    std::vector<int> objectives;       // objective values from all runs
    std::vector<int> phase1_lengths;   // cycle length after phase I (before removal)
    Solution best_solution;
    Stats stats;
    Stats phase1_stats;
};

// Algorithm function type: takes instance and start vertex, returns solution
using AlgoFunc = std::function<Solution(const Instance&, int)>;
// Two-phase algo: returns pair (phase1_objective, final_solution)
using AlgoFunc2 = std::function<std::pair<int, Solution>(const Instance&, int)>;

// Wrap a two-phase algorithm
inline AlgoFunc2 make_two_phase(
    std::function<Solution(const Instance&, int)> phase1_fn,
    bool do_phase2 = true
) {
    return [phase1_fn, do_phase2](const Instance& inst, int start) -> std::pair<int, Solution> {
        Solution sol = phase1_fn(inst, start);
        int phase1_obj = sol.compute_objective(inst);
        if (do_phase2) {
            phase2_removal(sol, inst);
        }
        return {phase1_obj, sol};
    };
}

inline ExperimentResult run_experiment(
    const Instance& inst,
    const std::string& algo_name,
    AlgoFunc2 algo,
    int num_runs = 200,
    std::mt19937* /*rng*/ = nullptr
) {
    ExperimentResult result;
    result.algo_name = algo_name;
    result.instance_name = inst.name;
    result.objectives.resize(num_runs);
    result.phase1_lengths.resize(num_runs);
    
    int best_obj = std::numeric_limits<int>::min();
    
    for (int run = 0; run < num_runs; run++) {
        int start = run % inst.n; // cycle through start vertices
        auto [phase1_obj, sol] = algo(inst, start);
        
        result.objectives[run] = sol.get_objective(inst);
        result.phase1_lengths[run] = phase1_obj;
        
        if (result.objectives[run] > best_obj) {
            best_obj = result.objectives[run];
            result.best_solution = sol;
        }
    }
    
    result.stats = compute_stats(result.objectives);
    result.phase1_stats = compute_stats(result.phase1_lengths);
    return result;
}

// Special run for random algorithm (no start vertex concept)
inline ExperimentResult run_random_experiment(
    const Instance& inst,
    std::mt19937& rng,
    int num_runs = 200
) {
    ExperimentResult result;
    result.algo_name = "Random";
    result.instance_name = inst.name;
    result.objectives.resize(num_runs);
    result.phase1_lengths.resize(num_runs);
    
    int best_obj = std::numeric_limits<int>::min();
    
    for (int run = 0; run < num_runs; run++) {
        Solution sol = random_solution(inst, rng);
        result.objectives[run] = sol.get_objective(inst);
        result.phase1_lengths[run] = result.objectives[run]; // no phase distinction
        
        if (result.objectives[run] > best_obj) {
            best_obj = result.objectives[run];
            result.best_solution = sol;
        }
    }
    
    result.stats = compute_stats(result.objectives);
    result.phase1_stats = result.stats;
    return result;
}

// Print results table
inline void print_results_table(const std::vector<std::vector<ExperimentResult>>& all_results,
                                 const std::vector<std::string>& instance_names) {
    int num_instances = (int)instance_names.size();
    
    // Header
    std::cout << std::left << std::setw(25) << "Algorithm";
    for (auto& name : instance_names) {
        std::cout << std::setw(35) << name;
    }
    std::cout << "\n" << std::string(25 + 35 * num_instances, '-') << "\n";
    
    // Each algorithm
    if (all_results.empty()) return;
    int num_algos = (int)all_results[0].size();
    
    for (int a = 0; a < num_algos; a++) {
        std::cout << std::left << std::setw(25) << all_results[0][a].algo_name;
        for (int i = 0; i < num_instances; i++) {
            auto& r = all_results[i][a];
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(0) << r.stats.avg
                << " (" << r.stats.min_val << " - " << r.stats.max_val << ")";
            std::cout << std::setw(35) << oss.str();
        }
        std::cout << "\n";
    }
    
    // Phase I table
    std::cout << "\n\nPhase I objectives (before vertex removal):\n";
    std::cout << std::left << std::setw(25) << "Algorithm";
    for (auto& name : instance_names) {
        std::cout << std::setw(35) << name;
    }
    std::cout << "\n" << std::string(25 + 35 * num_instances, '-') << "\n";
    
    for (int a = 0; a < num_algos; a++) {
        std::cout << std::left << std::setw(25) << all_results[0][a].algo_name;
        for (int i = 0; i < num_instances; i++) {
            auto& r = all_results[i][a];
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(0) << r.phase1_stats.avg
                << " (" << r.phase1_stats.min_val << " - " << r.phase1_stats.max_val << ")";
            std::cout << std::setw(35) << oss.str();
        }
        std::cout << "\n";
    }
}
