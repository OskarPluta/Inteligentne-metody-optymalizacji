#pragma once
#include "instance.h"
#include "objective.h"
#include <vector>

// Rozwiązanie: cykl Hamiltona na wybranym podzbiorze wierzchołków
struct Solution {
    std::vector<int> cycle; // indeksy wierzchołków w kolejności odwiedzania

    // Funkcja celu: node_value_sign * Σprofit + distance_sign * Σodległość
    // Domyślna konfiguracja: zysk − odległość (standard zadania)
    int objective(const Instance& inst,
                  const ObjectiveConfig& config = ObjectiveConfig{}) const;

    bool empty() const { return cycle.empty(); }
    int size() const { return static_cast<int>(cycle.size()); }
};
