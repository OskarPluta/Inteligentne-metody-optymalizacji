#pragma once
#include "../instance.h"
#include "../solution.h"
#include "../objective.h"
#include <vector>

// Mechanizm 2 z zad. 3 — lokalne przeszukiwanie stromy z ograniczeniem do
// ruchów kandydackich.
//
// Krawędzie kandydackie: dla każdego wierzchołka wyznaczamy jego K (=10
// domyślnie) najbliższych innych wierzchołków. Krawędź {u, v} jest
// kandydacka, jeżeli v jest na liście K najbliższych dla u lub odwrotnie.
//
// Ruch kandydacki: ruch wprowadzający do rozwiązania co najmniej jedną
// krawędź kandydacką. Sąsiedztwo: Insert + Remove + EdgeSwap (2-opt).
//
// KRYTYCZNE: pętla po ruchach jest sterowana listami sąsiadów (nie
// generujemy wszystkich ruchów żeby później filtrować). To warunek konieczny,
// żeby mechanizm faktycznie przyspieszał LS — patrz omowienie_zadania.md.

// Zwraca wektor list `nearest[v]` = indeksy K najbliższych wierzchołków do v
// (bez v), posortowanych rosnąco po odległości.
std::vector<std::vector<int>> compute_nearest(const Instance& inst, int k);

Solution local_search_cm(const Instance& inst, Solution init,
                         const std::vector<std::vector<int>>& nearest,
                         const ObjectiveConfig& config = ObjectiveConfig{});
