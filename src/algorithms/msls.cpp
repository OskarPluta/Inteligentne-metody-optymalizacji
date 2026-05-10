#include "msls.h"
#include "random_solution.h"
#include "local_search_cm.h"
#include <limits>
#include <utility>

Solution msls(const Instance& inst,
              const std::vector<std::vector<int>>& nearest,
              std::mt19937& rng,
              int iterations,
              const ObjectiveConfig& config) {
    Solution best;
    int best_obj = std::numeric_limits<int>::lowest();

    for (int it = 0; it < iterations; it++) {
        Solution init  = random_solution(inst, rng);
        Solution local = local_search_cm(inst, init, nearest, config);
        int obj = local.objective(inst, config);
        if (obj > best_obj) {
            best_obj = obj;
            best     = std::move(local);
        }
    }
    return best;
}
