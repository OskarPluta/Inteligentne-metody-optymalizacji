#include "include/types.hpp"
#include "include/phase2.hpp"
#include "include/algo_random.hpp"
#include "include/algo_nn.hpp"
#include "include/algo_gc.hpp"
#include "include/experiment.hpp"
#include "include/visualization.hpp"

int main(int argc, char* argv[]) {
    // Default instance files - can be overridden by command line args
    std::string file_a = "TSPA.csv";
    std::string file_b = "TSPB.csv";
    int num_runs = 200;

    if (argc >= 3) {
        file_a = argv[1];
        file_b = argv[2];
    }
    if (argc >= 4) {
        num_runs = std::atoi(argv[3]);
    }

    // Load instances
    Instance inst_a, inst_b;
    inst_a.load(file_a, "TSPA");
    inst_b.load(file_b, "TSPB");

    std::cout << "Loaded " << inst_a.name << ": " << inst_a.n << " nodes\n";
    std::cout << "Loaded " << inst_b.name << ": " << inst_b.n << " nodes\n\n";

    std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());

    std::vector<Instance*> instances = {&inst_a, &inst_b};
    std::vector<std::string> instance_names = {inst_a.name, inst_b.name};

    // Define algorithms as (name, two-phase function)
    struct AlgoEntry {
        std::string name;
        AlgoFunc2 func;
        bool is_random;
    };

    std::vector<AlgoEntry> algos = {
        {"Random", {}, true},
        {"NNa (no profit)", make_two_phase([](const Instance& inst, int s) {
            return nearest_neighbor(inst, s, false);
        }), false},
        {"NN (with profit)", make_two_phase([](const Instance& inst, int s) {
            return nearest_neighbor(inst, s, true);
        }), false},
        {"GCa (no profit)", make_two_phase([](const Instance& inst, int s) {
            return greedy_cycle_phase1(inst, s, GCVariant::GC_NO_PROFIT);
        }), false},
        {"GC (with profit)", make_two_phase([](const Instance& inst, int s) {
            return greedy_cycle_phase1(inst, s, GCVariant::GC_WITH_PROFIT);
        }), false},
        {"2-Regret", make_two_phase([](const Instance& inst, int s) {
            return greedy_cycle_phase1(inst, s, GCVariant::REGRET_2);
        }), false},
        {"Weighted 2-Regret", make_two_phase([](const Instance& inst, int s) {
            return greedy_cycle_phase1(inst, s, GCVariant::WEIGHTED_REGRET_2);
        }), false},
    };

    // Run experiments
    // all_results[instance_idx][algo_idx]
    std::vector<std::vector<ExperimentResult>> all_results;

    for (int i = 0; i < (int)instances.size(); i++) {
        const Instance& inst = *instances[i];
        std::vector<ExperimentResult> inst_results;

        for (auto& algo : algos) {
            std::cout << "Running " << algo.name << " on " << inst.name << "...\n";
            
            ExperimentResult result;
            if (algo.is_random) {
                result = run_random_experiment(inst, rng, num_runs);
            } else {
                result = run_experiment(inst, algo.name, algo.func, num_runs);
            }
            
            inst_results.push_back(result);
        }

        all_results.push_back(inst_results);
    }

    // Print results
    std::cout << "\n\n========== FINAL RESULTS (after Phase II) ==========\n\n";
    print_results_table(all_results, instance_names);

    // Save best solutions as SVG visualizations
    std::cout << "\n\nSaving visualizations...\n";
    for (int i = 0; i < (int)instances.size(); i++) {
        for (int a = 0; a < (int)all_results[i].size(); a++) {
            auto& r = all_results[i][a];
            std::string fname = "viz_" + instance_names[i] + "_" + std::to_string(a) + ".svg";
            // Clean up filename
            for (char& c : fname) {
                if (c == ' ' || c == '(' || c == ')') c = '_';
            }
            std::string title = r.algo_name + " on " + instance_names[i]
                              + " (obj=" + std::to_string(r.stats.max_val) + ")";
            save_solution_svg(r.best_solution, *instances[i], fname, title);
            std::cout << "  " << fname << "\n";
        }
    }

    // Save best solutions as CSV (for SolutionChecker or further processing)
    for (int i = 0; i < (int)instances.size(); i++) {
        for (int a = 0; a < (int)all_results[i].size(); a++) {
            auto& r = all_results[i][a];
            std::string fname = "sol_" + instance_names[i] + "_" + std::to_string(a) + ".txt";
            for (char& c : fname) {
                if (c == ' ' || c == '(' || c == ')') c = '_';
            }
            r.best_solution.save(fname, *instances[i]);
        }
    }

    std::cout << "\nDone.\n";
    return 0;
}
