#include "src/instance.h"
#include "src/solution.h"
#include "src/objective.h"
#include "src/experiment.h"
#include "src/algorithms/random_solution.h"
#include "src/algorithms/nn.h"
#include "src/algorithms/gc.h"
#include "src/algorithms/regret.h"
#include "src/algorithms/local_search.h"
#include "src/algorithms/random_walk.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <filesystem>
#include <random>
#include <string>
#include <vector>

using NamedResult = std::pair<std::string, ExperimentResult>;

// Uruchamia wszystkie algorytmy na danej instancji, wypisuje i zwraca wyniki.
// Zwraca parę: (wyniki po fazie I+II, wyniki tylko fazy I).
using ResultPair = std::pair<std::vector<NamedResult>, std::vector<NamedResult>>;

ResultPair run_all(const Instance& inst, const std::string& label,
                   const ObjectiveConfig& config) {
    std::vector<NamedResult> results, phase1_results;

    auto add = [&](const std::string& name, auto fn) {
        std::cerr << "[" << label << "] " << name << "\n";
        auto res = run_experiment(inst, fn);
        results.emplace_back(name, res);
    };
    auto add_p1 = [&](const std::string& name, auto fn) {
        std::cerr << "[" << label << "] " << name << " (phase1)\n";
        auto res = run_experiment(inst, fn);
        phase1_results.emplace_back(name, res);
    };

    add("Random",   [&](int start) {
        std::mt19937 rng(start);
        return random_solution(inst, rng);
    });

    // Faza I+II
    add("NNa",      [&](int start) { return nearest_neighbor(inst, start, false, config); });
    add("NN",       [&](int start) { return nearest_neighbor(inst, start, true,  config); });
    add("GCa",      [&](int start) { return greedy_cycle(inst, start, false, config); });
    add("GC",       [&](int start) { return greedy_cycle(inst, start, true,  config); });
    add("2-regret", [&](int start) { return regret2(inst, start, config); });

    // Lokalne przeszukiwanie — 8 kombinacji (tryb × sąsiedztwo × start).
    // "Najlepsza heurystyka" → 2-regret (patrz sprawozdanie z zad. 1).
    auto add_ls = [&](const std::string& name,
                      LSMode mode, LSNeighborhood neigh, bool heur_start) {
        add(name, [&, mode, neigh, heur_start](int start) {
            std::mt19937 rng(static_cast<unsigned>(start) * 2654435761u
                             + (heur_start ? 1u : 0u));
            Solution init = heur_start
                ? regret2(inst, start, config)
                : random_solution(inst, rng);
            return local_search(inst, init, mode, neigh, rng, config);
        });
    };

    add_ls("LSs_v_rnd", LSMode::Steepest, LSNeighborhood::NodeSwap, false);
    add_ls("LSs_v_h",   LSMode::Steepest, LSNeighborhood::NodeSwap, true);
    add_ls("LSs_e_rnd", LSMode::Steepest, LSNeighborhood::EdgeSwap, false);
    add_ls("LSs_e_h",   LSMode::Steepest, LSNeighborhood::EdgeSwap, true);
    add_ls("LSg_v_rnd", LSMode::Greedy,   LSNeighborhood::NodeSwap, false);
    add_ls("LSg_v_h",   LSMode::Greedy,   LSNeighborhood::NodeSwap, true);
    add_ls("LSg_e_rnd", LSMode::Greedy,   LSNeighborhood::EdgeSwap, false);
    add_ls("LSg_e_h",   LSMode::Greedy,   LSNeighborhood::EdgeSwap, true);

    // Błądzenie losowe — punkt odniesienia. Budżet czasu = średni czas
    // najwolniejszej wersji LS (po wszystkich 8 wariantach na tej instancji).
    double rw_budget = 0.0;
    std::string slowest_ls;
    for (const auto& [name, res] : results) {
        if (name.rfind("LS", 0) == 0 && res.avg_time > rw_budget) {
            rw_budget  = res.avg_time;
            slowest_ls = name;
        }
    }
    std::cerr << "[" << label << "] RW budget = " << rw_budget
              << " s/uruchomienie (najwolniejszy LS: " << slowest_ls << ")\n";
    add("RW", [&, rw_budget](int start) {
        std::mt19937 rng(static_cast<unsigned>(start) * 2654435761u + 7u);
        Solution init = random_solution(inst, rng);
        return random_walk(inst, init, rw_budget, rng, config);
    });

    // Tylko faza I (do tabeli w sprawozdaniu)
    add_p1("NNa-I",      [&](int start) { return nn_phase1(inst, start, false, config); });
    add_p1("NN-I",       [&](int start) { return nn_phase1(inst, start, true,  config); });
    add_p1("GCa-I",      [&](int start) { return gc_phase1(inst, start, false, config); });
    add_p1("GC-I",       [&](int start) { return gc_phase1(inst, start, true,  config); });
    add_p1("2-regret-I", [&](int start) { return regret_phase1(inst, start, config); });

    std::cout << "=== " << label << " (" << inst.n() << " wierzchołków) ===\n";
    for (const auto& [name, res] : results) print_result(name, res);
    std::cout << "--- Faza I ---\n";
    for (const auto& [name, res] : phase1_results) print_result(name, res);
    std::cout << "\n";

    return {results, phase1_results};
}

// Zamienia '-' i spacje na '_' (bezpieczna nazwa pliku).
static std::string safe_name(const std::string& s) {
    std::string r = s;
    for (char& c : r) if (c == '-' || c == ' ') c = '_';
    return r;
}

// Zapisuje wierzchołki instancji i najlepsze ścieżki każdego algorytmu.
void save_paths(const std::string& dir, const Instance& inst,
                const std::string& inst_name,
                const std::vector<NamedResult>& results) {
    namespace fs = std::filesystem;
    fs::create_directories(dir);

    // Wszystkie wierzchołki (tło mapy)
    {
        std::ofstream f(dir + "/" + inst_name + "_nodes.csv");
        f << "id,x,y,profit\n";
        for (int i = 0; i < inst.n(); i++)
            f << i << "," << inst.nodes[i].x << ","
              << inst.nodes[i].y << "," << inst.nodes[i].profit << "\n";
    }

    // Najlepsza ścieżka każdego algorytmu (w kolejności cyklu)
    for (const auto& [name, res] : results) {
        std::ofstream f(dir + "/" + inst_name + "_" + safe_name(name) + "_path.csv");
        f << "step,node_id,x,y,profit\n";
        for (int step = 0; step < res.best.size(); step++) {
            int idx = res.best.cycle[step];
            f << step << "," << idx << ","
              << inst.nodes[idx].x << "," << inst.nodes[idx].y << ","
              << inst.nodes[idx].profit << "\n";
        }
    }
}

// Zapisuje indeksy cyklu w formacie gotowym do wklejenia do kolumny F
// w Solution checker2.xlsx (jeden node_id na linię).
void save_checker(const std::string& dir, const std::string& inst_name,
                  const std::vector<NamedResult>& results) {
    namespace fs = std::filesystem;
    fs::create_directories(dir);
    for (const auto& [name, res] : results) {
        std::ofstream f(dir + "/checker_" + inst_name + "_" + safe_name(name) + ".txt");
        for (int i = 0; i < res.best.size(); i++)
            f << res.best.cycle[i] << "\n";
    }
}

// Zapisuje podsumowanie wszystkich eksperymentów do jednego pliku CSV.
void save_summary(const std::string& path,
                  const std::vector<std::pair<std::string, std::vector<NamedResult>>>& all) {
    std::ofstream f(path);
    f << "instance,algorithm,min_obj,max_obj,avg_obj,min_time_s,max_time_s,avg_time_s,best_start,nodes_in_best\n";
    for (const auto& [inst_name, results] : all)
        for (const auto& [name, res] : results)
            f << inst_name << "," << name << ","
              << res.min_obj << "," << res.max_obj << ","
              << std::fixed << std::setprecision(1) << res.avg_obj << ","
              << std::setprecision(6)
              << res.min_time << "," << res.max_time << "," << res.avg_time << ","
              << res.best_start << "," << res.best.size() << "\n";
}

int main() {
    ObjectiveConfig config = ObjectiveConfig::profit();

    Instance tspa = Instance::load("TSPA.csv");
    Instance tspb = Instance::load("TSPB.csv");

    auto [res_a, p1_a] = run_all(tspa, "TSPA.csv", config);
    auto [res_b, p1_b] = run_all(tspb, "TSPB.csv", config);

    save_paths("results", tspa, "TSPA", res_a);
    save_paths("results", tspb, "TSPB", res_b);
    save_checker("results", "TSPA", res_a);
    save_checker("results", "TSPB", res_b);
    save_summary("results/results.csv", {{"TSPA", res_a}, {"TSPB", res_b}});
    save_summary("results/results_phase1.csv", {{"TSPA", p1_a}, {"TSPB", p1_b}});

    std::cout << "Dane zapisane do katalogu results/\n";
    return 0;
}
