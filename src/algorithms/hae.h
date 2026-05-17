#pragma once
#include "../instance.h"
#include "../solution.h"
#include "../objective.h"
#include <random>
#include <vector>

// Hybrydowy algorytm ewolucyjny (HAE) — zad. 6.
//
// Elitarna populacja (domyślnie 20) z modelem steady-state. W populacji
// nie ma duplikatów porównywanych po wartości funkcji celu.
//
// Pseudokod:
//   Wygeneruj populację początkową X (LP na losowych rozwiązaniach, 20 różnych)
//   Powtarzaj dopóki czas < time_budget_s:
//       Wylosuj dwóch różnych rodziców (rozkład równomierny)
//       y := Recombine(op, A, B)
//       y := Repair(y) (2-regret do |A|)  +  Phase2 (usuwanie wierzchołków)
//       Jeżeli use_local_search to y := LokalnePrzeszukiwanie(y)
//       Jeżeli f(y) > f(najgorszy) i f(y) ∉ wartości w populacji:
//           Zastąp najgorszego przez y
//   Zwróć najlepszego z populacji
//
// Operatory rekombinacji:
//   Op1 — zachowuje wspólne wierzchołki i krawędzie obu rodziców; łączy
//         powstałe podścieżki losowo (kolejność + ew. odwrócenie).
//   Op2 — bierze rodzica A jako bazę; usuwa wierzchołki i krawędzie nie
//         występujące u rodzica B; powstałe podścieżki łączy losowo.
//   Op3 — bierze rodzica A jako bazę; usuwa tylko wierzchołki nie występujące
//         u rodzica B (zamykając luki krawędziami między sąsiadami).
//
// Po rekombinacji zawsze stosujemy 2-regret repair + Fazę II (usuwanie
// wierzchołków poprawiające funkcję celu), zgodnie z opisem LNS w zad. 4/6.
//
// Po zakończeniu, do `iters_out` zapisywana jest liczba iteracji
// steady-state (po inicjalizacji populacji).
enum class HaeOperator { Op1 = 1, Op2 = 2, Op3 = 3 };

Solution hae(const Instance& inst,
             const std::vector<std::vector<int>>& nearest,
             std::mt19937& rng,
             double time_budget_s,
             int& iters_out,
             HaeOperator op,
             bool use_local_search = true,
             int pop_size = 20,
             const ObjectiveConfig& config = ObjectiveConfig{});
