#include "random_solution.h"
#include <numeric>    // std::iota
#include <algorithm>  // std::shuffle

Solution random_solution(const Instance& inst, std::mt19937& rng) {
    int n = inst.n();

    // Krok 1: losujemy liczbę wybranych wierzchołków
    std::uniform_int_distribution<int> size_dist(2, n);
    int k = size_dist(rng);

    // Krok 2: tworzymy listę wszystkich indeksów i mieszamy ją
    std::vector<int> indices(n);
    std::iota(indices.begin(), indices.end(), 0); // [0, 1, 2, ..., n-1]
    std::shuffle(indices.begin(), indices.end(), rng);

    // Krok 3: bierzemy pierwsze k indeksów — to są losowo wybrane i losowo
    // ustawione wierzchołki (shuffle już zadba o losową kolejność)
    Solution sol;
    sol.cycle.assign(indices.begin(), indices.begin() + k);
    return sol;
}

/*
PSEUDOKOD:
n := liczba wierzchołków
losuj k z przedziału [2, n]
indices := [0, 1, 2, ..., n-1]
wymieszaj losowo indices
cykl := pierwsze k elementów z indices
*/