#pragma once
#include "instance.h"
#include "solution.h"
#include <functional>
#include <string>

// Statystyki liczby iteracji "zewnętrznych" algorytmu (np. perturbacji ILS,
// destroy-repair LNS, steady-state HAE). Pole `present` rozróżnia "brak danych"
// (algorytm nie raportuje iteracji) od "0 iteracji w żadnym uruchomieniu".
struct IterStats {
    double avg     = 0.0;
    int    mn      = 0;
    int    mx      = 0;
    bool   present = false;
};

struct ExperimentResult {
    int       min_obj;
    int       max_obj;
    double    avg_obj;
    Solution  best;       // rozwiązanie z najwyższą wartością f.celu
    int       best_start; // wierzchołek startowy który dał najlepszy wynik

    // Czas pojedynczego uruchomienia algorytmu (sekundy).
    double    min_time;
    double    max_time;
    double    avg_time;

    // Liczba iteracji zewnętrznych (opcjonalna — wypełniana ręcznie po
    // uruchomieniu eksperymentu, gdy algorytm ją raportuje).
    IterStats iters;
};

// Uruchamia algorytm num_runs razy, losując wierzchołki startowe z [0, n).
// algo(start) powinno zwrócić gotowe rozwiązanie (po obu fazach).
ExperimentResult run_experiment(
    const Instance& inst,
    const std::function<Solution(int start)>& algo,
    int num_runs = 100);

// Wypisuje wyniki eksperymentu w czytelnym formacie.
void print_result(const std::string& name, const ExperimentResult& res);
