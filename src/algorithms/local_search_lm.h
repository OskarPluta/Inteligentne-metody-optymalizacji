#pragma once
#include "../instance.h"
#include "../solution.h"
#include "../objective.h"
#include <random>

// Lokalne przeszukiwanie w wersji stromej z wykorzystaniem ocen ruchów
// z poprzednich iteracji (uporządkowana lista ruchów LM).
//
// Sąsiedztwo: Insert + Remove (między-trasowe) + EdgeSwap (wewnątrz-trasowy).
// Wybór EdgeSwap motywowany wynikami z zadania 2 (lepsze niż NodeSwap).
//
// Implementacja zgodna z wersją alternatywną zaleconą przez prowadzącego:
//   1. inicjalizacja LM rozwiązania startowego — wszystkie ruchy poprawiające,
//   2. w pętli: szukaj w LM od najlepszego ruchu aplikowalnego;
//      ruchy jednoznacznie nieważne usuwamy, ruchy z odwróconym
//      kierunkiem krawędzi pomijamy ale pozostawiamy w LM,
//   3. gdy zaaplikujemy ruch — kontynuujemy z bieżącym LM,
//   4. gdy żaden ruch w LM nie jest aplikowalny — uzupełniamy LM o nowe ruchy
//      poprawiające (uwzględniając obie orientacje krawędzi dla EdgeSwap),
//   5. kończymy gdy uzupełnienie nie wnosi żadnego nowego ruchu.
Solution local_search_lm(const Instance& inst, Solution init,
                         const ObjectiveConfig& config = ObjectiveConfig{});
