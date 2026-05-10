#pragma once
#include "../instance.h"
#include "../solution.h"
#include "../objective.h"
#include <random>
#include <vector>

// Multiple Start Local Search (MSLS) — zad. 4.
//
// Pseudokod:
//   Powtarzaj `iterations` razy:
//       Wygeneruj losowe rozwiązanie startowe x
//       x := Lokalne przeszukiwanie(x)
//       Jeżeli f(x) > f(best) to best := x
//   Zwróć best
//
// Jako lokalne przeszukiwanie stosujemy `local_search_cm` (LS stromy
// z ruchami kandydackimi K=10) — najlepsza metoda LS po średniej
// z poprzednich zajęć (TSPA: 5854.4, TSPB: 17384.2).
//
// Domyślnie 200 iteracji (zgodnie z wymaganiem zadania).
Solution msls(const Instance& inst,
              const std::vector<std::vector<int>>& nearest,
              std::mt19937& rng,
              int iterations = 200,
              const ObjectiveConfig& config = ObjectiveConfig{});
