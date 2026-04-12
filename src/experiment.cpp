#include "experiment.h"
#include <limits>
#include <iostream>
#include <iomanip>
#include <chrono>

ExperimentResult run_experiment(
    const Instance& inst,
    const std::function<Solution(int start)>& algo)
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

    for (int start = 0; start < n; start++) {
        if (start % 10 == 0) {
            std::cerr << "\r  [" << start << "/" << n << "]   " << std::flush;
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

    std::cerr << "\r  [" << n << "/" << n << "]   \n";
    res.avg_obj  /= n;
    res.avg_time /= n;
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
