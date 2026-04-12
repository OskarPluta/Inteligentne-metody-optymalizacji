#pragma once
#include "../instance.h"
#include "../solution.h"
#include "../objective.h"
#include <random>

// Lokalne przeszukiwanie.
//
// Sąsiedztwo zawsze składa się z dwóch grup ruchów:
//   1. Ruchy zmieniające zbiór wybranych wierzchołków:
//      - Insert — dodanie wierzchołka spoza cyklu w wybranej pozycji
//      - Remove — usunięcie wierzchołka z cyklu
//   2. Ruchy wewnątrztrasowe — jedno z dwóch (wybór przez LSNeighborhood):
//      - NodeSwap — zamiana dwóch wierzchołków w cyklu
//      - EdgeSwap — 2-opt, odwrócenie segmentu (wymiana dwóch krawędzi)
//
// Tryb:
//   - Steepest — przegląda wszystkie ruchy i wybiera najlepszy w każdej iteracji
//   - Greedy   — przegląda ruchy w losowej (przemieszanej) kolejności i stosuje
//                pierwszy poprawiający; kończy gdy po całym przejrzeniu M(x)
//                żaden ruch nie poprawia rozwiązania.
//
// Implementacja wykorzystuje wyłącznie obliczanie delty funkcji celu
// (delta > 0 oznacza poprawę, bo objective jest maksymalizowany).
enum class LSMode        { Steepest, Greedy };
enum class LSNeighborhood{ NodeSwap, EdgeSwap };

Solution local_search(const Instance& inst, Solution init,
                      LSMode mode, LSNeighborhood neigh,
                      std::mt19937& rng,
                      const ObjectiveConfig& config = ObjectiveConfig{});
