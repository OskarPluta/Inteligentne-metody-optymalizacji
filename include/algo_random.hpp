#pragma once
#include "types.hpp"

// Random solution: pick random number of vertices, random vertices, random order
inline Solution random_solution(const Instance& inst, std::mt19937& rng) {
    Solution sol;
    int n = inst.n;

    // Random number of vertices [3, n]
    std::uniform_int_distribution<int> count_dist(3, n);
    int k = count_dist(rng);

    // Create shuffled indices and take first k
    std::vector<int> indices(n);
    std::iota(indices.begin(), indices.end(), 0);
    std::shuffle(indices.begin(), indices.end(), rng);
    
    sol.cycle.assign(indices.begin(), indices.begin() + k);
    // Already in random order due to shuffle
    sol.compute_objective(inst);
    return sol;
}
