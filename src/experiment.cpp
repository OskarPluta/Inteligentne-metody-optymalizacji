#include "experiment.h"
#include <limits>
#include <iostream>
#include <iomanip>

ExperimentResult run_experiment(
    const Instance& inst,
    const std::function<Solution(int start)>& algo)
{
    ExperimentResult res;
    res.min_obj   = std::numeric_limits<int>::max();
    res.max_obj   = std::numeric_limits<int>::lowest();
    res.avg_obj   = 0.0;
    res.best_start = -1;

    int n = inst.n();

    for (int start = 0; start < n; start++) {
        Solution sol = algo(start);
        int obj = sol.objective(inst);

        if (obj < res.min_obj) res.min_obj = obj;
        if (obj > res.max_obj) {
            res.max_obj    = obj;
            res.best       = sol;
            res.best_start = start;
        }
        res.avg_obj += obj;
    }

    res.avg_obj /= n;
    return res;
}

void print_result(const std::string& name, const ExperimentResult& res)
{
    std::cout << std::left  << std::setw(12) << name
              << "  avg=" << std::fixed << std::setprecision(1)
              << std::setw(10) << res.avg_obj
              << "  min=" << std::setw(8) << res.min_obj
              << "  max=" << std::setw(8) << res.max_obj
              << "  (best start=" << res.best_start << ")\n";
}
