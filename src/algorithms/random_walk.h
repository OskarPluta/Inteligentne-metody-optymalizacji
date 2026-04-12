#pragma once
#include "../instance.h"
#include "../solution.h"
#include "../objective.h"
#include <random>

// Błądzenie losowe (random walk) — punkt odniesienia dla lokalnego przeszukiwania.
//
// Zaczyna od losowego rozwiązania, w każdej iteracji wykonuje losowo wybrany ruch
// (Insert / Remove / NodeSwap / EdgeSwap — sąsiedztwo identyczne jak w LS, łącznie
// oba typy wewnątrztrasowe), niezależnie od jego oceny. Zwraca najlepsze
// rozwiązanie napotkane w trakcie błądzenia.
//
// Algorytm działa do wyczerpania budżetu czasu `time_budget_s` (sekundy).
// Budżet powinien odpowiadać średniemu czasowi najwolniejszej wersji LS.
Solution random_walk(const Instance& inst, Solution init,
                     double time_budget_s,
                     std::mt19937& rng,
                     const ObjectiveConfig& config = ObjectiveConfig{});
