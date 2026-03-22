#pragma once
#include "../instance.h"
#include "../solution.h"
#include "../objective.h"

// Faza I — buduje cykl Hamiltona na WSZYSTKICH wierzchołkach metodą NN.
//
// use_profit=false → NNa: wybieramy wierzchołek z minimalną odległością od ostatniego
// use_profit=true  → NN:  wybieramy wierzchołek maksymalizujący
//                         node_value_sign*profit[v] − dist[last][v]
Solution nn_phase1(const Instance& inst, int start, bool use_profit,
                   const ObjectiveConfig& config = ObjectiveConfig{});

// Pełny algorytm NN = faza I + faza II (usuwanie wierzchołków).
Solution nearest_neighbor(const Instance& inst, int start, bool use_profit,
                           const ObjectiveConfig& config = ObjectiveConfig{});
