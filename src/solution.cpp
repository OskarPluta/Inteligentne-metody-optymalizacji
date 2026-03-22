#include "solution.h"

int Solution::objective(const Instance& inst, const ObjectiveConfig& config) const {
    if (cycle.size() < 2) return 0;

    int total_node_value = 0;
    int total_distance   = 0;
    int n = size();

    for (int i = 0; i < n; i++) {
        int curr = cycle[i];
        int next = cycle[(i + 1) % n]; // % n zamyka cykl (ostatni → pierwszy)

        total_node_value += inst.nodes[curr].profit;
        total_distance   += inst.dist[curr][next];
    }

    // Wzór: node_value_sign * Σ(trzecia_kolumna) − Σ(odległość)
    // Odległość jest zawsze kosztem (zawsze odejmujemy).
    return config.node_value_sign * total_node_value - total_distance;
}
