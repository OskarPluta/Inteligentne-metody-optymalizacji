#pragma once
#include "../instance.h"
#include "../solution.h"
#include "../objective.h"
#include <random>
#include <vector>

// Large Neighborhood Search (LNS) — zad. 4.
//
// Pseudokod:
//   x := LokalnePrzeszukiwanie(LosoweRozwiązanie())
//   best := x
//   Powtarzaj dopóki czas < time_budget_s:
//       y := x
//       Destroy(y)                    (~30% wierzchołków)
//       Repair(y)                     (2-regret — najlepsza heurystyka z lab1)
//       Jeżeli use_local_search to:
//           y := LokalnePrzeszukiwanie(y)
//       Jeżeli f(y) > f(x) to x := y
//       Jeżeli f(y) > f(best) to best := y
//   Zwróć best
//
// Destroy: ważone losowanie wierzchołków do usunięcia — preferujemy
//   długie sąsiednie krawędzie i niskie zyski. Waga(v) =
//   dist[prev][v] + dist[v][next] − node_value_sign·profit[v].
//
// Repair: 2-regret na wierzchołkach spoza cyklu, do osiągnięcia
//   docelowego rozmiaru (= rozmiar sprzed destroy).
//
// `use_local_search == false` daje wersję LNSa (bez LS w pętli;
// LS aplikowane tylko na rozwiązanie startowe — tu i tak losowe).
//
// Po zakończeniu, do `iters_out` zapisywana jest liczba zewnętrznych
// iteracji destroy+repair (do raportu).
Solution lns(const Instance& inst,
             const std::vector<std::vector<int>>& nearest,
             std::mt19937& rng,
             double time_budget_s,
             int& iters_out,
             bool use_local_search = true,
             double destroy_fraction = 0.30,
             const ObjectiveConfig& config = ObjectiveConfig{});
