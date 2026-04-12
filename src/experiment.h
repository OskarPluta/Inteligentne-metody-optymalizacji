#pragma once
#include "instance.h"
#include "solution.h"
#include <functional>
#include <string>

struct ExperimentResult {
    int      min_obj;
    int      max_obj;
    double   avg_obj;
    Solution best;       // rozwiązanie z najwyższą wartością f.celu
    int      best_start; // wierzchołek startowy który dał najlepszy wynik

    // Czas pojedynczego uruchomienia algorytmu (sekundy).
    double   min_time;
    double   max_time;
    double   avg_time;
};

// Uruchamia algorytm num_runs razy, losując wierzchołki startowe z [0, n).
// algo(start) powinno zwrócić gotowe rozwiązanie (po obu fazach).
ExperimentResult run_experiment(
    const Instance& inst,
    const std::function<Solution(int start)>& algo,
    int num_runs = 100);

// Wypisuje wyniki eksperymentu w czytelnym formacie.
void print_result(const std::string& name, const ExperimentResult& res);
