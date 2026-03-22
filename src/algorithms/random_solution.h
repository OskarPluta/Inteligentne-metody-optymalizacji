#pragma once
#include "../instance.h"
#include "../solution.h"
#include <random>

// Tworzy losowe rozwiązanie:
//   1. Losuje liczbę wierzchołków k ∈ [2, n]
//   2. Losuje k wierzchołków spośród wszystkich
//   3. Układa je w losowej kolejności (= losowy cykl)
Solution random_solution(const Instance& inst, std::mt19937& rng);
