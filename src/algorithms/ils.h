#pragma once
#include "../instance.h"
#include "../solution.h"
#include "../objective.h"
#include <random>
#include <vector>

// Iterated Local Search (ILS) — zad. 4.
//
// Pseudokod:
//   x := LosoweRozwiązanie()
//   x := LokalnePrzeszukiwanie(x)
//   best := x
//   Powtarzaj dopóki czas < time_budget_s:
//       y := x
//       Perturbacja(y)               (mała — kilka ruchów)
//       y := LokalnePrzeszukiwanie(y)
//       Jeżeli f(y) > f(x) to x := y
//       Jeżeli f(y) > f(best) to best := y
//   Zwróć best
//
// Lokalne przeszukiwanie: `local_search_cm` (najlepsze z zad. 3).
// Warunek stopu: średni czas MSLS na tej samej instancji.
//
// Perturbacja (zob. ils.cpp): złożenie K=4 losowych ruchów; każdy ruch
// to losowo:
//   - zamiana wierzchołka w cyklu na losowy spoza cyklu (zachowuje |y|),
//   - losowy 2-opt (odwrócenie losowego segmentu).
//
// Po zakończeniu, do `iters_out` zapisywana jest liczba zewnętrznych
// iteracji perturbacji+LS (do raportu).
Solution ils(const Instance& inst,
             const std::vector<std::vector<int>>& nearest,
             std::mt19937& rng,
             double time_budget_s,
             int& iters_out,
             const ObjectiveConfig& config = ObjectiveConfig{});
