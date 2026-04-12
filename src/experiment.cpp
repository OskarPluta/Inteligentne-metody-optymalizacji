#include "experiment.h"
#include <limits>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <algorithm>
#include <numeric>
#include <random>

ExperimentResult run_experiment(
    const Instance& inst,
    const std::function<Solution(int start)>& algo,
    int num_runs)
{
    ExperimentResult res;
    res.min_obj    = std::numeric_limits<int>::max();
    res.max_obj    = std::numeric_limits<int>::lowest();
    res.avg_obj    = 0.0;
    res.best_start = -1;
    res.min_time   = std::numeric_limits<double>::infinity();
    res.max_time   = 0.0;
    res.avg_time   = 0.0;

    int n = inst.n();

    // Przygotuj listę wierzchołków startowych: losujemy num_runs z [0, n).
    std::vector<int> starts(n);
    std::iota(starts.begin(), starts.end(), 0);
    std::mt19937 start_rng(42);
    std::shuffle(starts.begin(), starts.end(), start_rng);
    if (num_runs < n) starts.resize(num_runs);

    for (int i = 0; i < static_cast<int>(starts.size()); i++) {
        int start = starts[i];
        if (i % 10 == 0) {
            std::cerr << "\r  [" << i << "/" << num_runs << "]   " << std::flush;
        }
        auto t0 = std::chrono::steady_clock::now();
        Solution sol = algo(start);
        auto t1 = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(t1 - t0).count();

        int obj = sol.objective(inst);

        if (obj < res.min_obj) res.min_obj = obj;
        if (obj > res.max_obj) {
            res.max_obj    = obj;
            res.best       = sol;
            res.best_start = start;
        }
        res.avg_obj += obj;

        if (elapsed < res.min_time) res.min_time = elapsed;
        if (elapsed > res.max_time) res.max_time = elapsed;
        res.avg_time += elapsed;
    }

    std::cerr << "\r  [" << num_runs << "/" << num_runs << "]   \n";
    res.avg_obj  /= starts.size();
    res.avg_time /= starts.size();
    return res;
}

void print_result(const std::string& name, const ExperimentResult& res)
{
    std::cout << std::left  << std::setw(12) << name
              << "  avg=" << std::fixed << std::setprecision(1)
              << std::setw(10) << res.avg_obj
              << "  min=" << std::setw(8) << res.min_obj
              << "  max=" << std::setw(8) << res.max_obj
              << "  t_avg=" << std::setprecision(4) << std::setw(8) << res.avg_time << "s"
              << "  (best start=" << res.best_start << ")\n";
}
