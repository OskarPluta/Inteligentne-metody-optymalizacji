#pragma once
#include "../instance.h"
#include "../solution.h"
#include "../objective.h"

// Faza II — wspólna dla NN i GC:
// Zachłannie usuwa wierzchołki poprawiające funkcję celu,
// dopóki istnieje wierzchołek którego usunięcie przynosi poprawę.
//
// Delta usunięcia wierzchołka v (między prev i next):
//   delta = −node_value_sign * profit[v]
//           + dist[prev][v] + dist[v][next] − dist[prev][next]
// Usuwamy gdy delta > 0 (poprawa funkcji celu).
Solution phase2_remove(const Solution& sol, const Instance& inst,
                        const ObjectiveConfig& config = ObjectiveConfig{});
