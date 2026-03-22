#pragma once
#include "../instance.h"
#include "../solution.h"
#include "../objective.h"

// Faza I — buduje cykl Hamiltona na WSZYSTKICH wierzchołkach metodą rozbudowy cyklu.
//
// use_profit=false → GCa: wstawiamy wierzchołek powodujący najmniejszy wzrost odległości
// use_profit=true  → GC:  wstawiamy wierzchołek maksymalizujący
//                         node_value_sign*profit[v] − delta_dist
//
// delta_dist wstawienia v między cycle[i] a cycle[i+1]:
//   delta_dist = dist[i][v] + dist[v][i+1] − dist[i][i+1]
Solution gc_phase1(const Instance& inst, int start, bool use_profit,
                   const ObjectiveConfig& config = ObjectiveConfig{});

// Pełny algorytm GC = faza I + faza II (usuwanie wierzchołków).
Solution greedy_cycle(const Instance& inst, int start, bool use_profit,
                      const ObjectiveConfig& config = ObjectiveConfig{});
