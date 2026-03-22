#pragma once
#include "../instance.h"
#include "../solution.h"
#include "../objective.h"

// Faza I — 2-regret heuristic na bazie rozbudowy cyklu.
//
// Zamiast zachłannie wstawiać wierzchołek dający największą poprawę,
// wybieramy wierzchołek o największym 2-żalu:
//
//   2-regret(v) = delta_dist_2nd(v) - delta_dist_1st(v)
//
// gdzie delta_dist_k(v) to k-ty najmniejszy wzrost długości cyklu
// przy wstawieniu v. Im większy żal, tym bardziej v "traci" jeśli
// nie wstawimy go teraz (drugie najlepsze miejsce jest dużo gorsze).
//
// Wybrany wierzchołek wstawiamy w jego najlepsze miejsce (1st).
Solution regret_phase1(const Instance& inst, int start,
                       const ObjectiveConfig& config = ObjectiveConfig{});

// Pełny algorytm 2-regret = faza I + faza II (usuwanie wierzchołków).
Solution regret2(const Instance& inst, int start,
                 const ObjectiveConfig& config = ObjectiveConfig{});
