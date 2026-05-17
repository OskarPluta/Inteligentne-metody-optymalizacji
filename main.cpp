#include "src/instance.h"
#include "src/solution.h"
#include "src/objective.h"
#include "src/experiment.h"
#include "src/algorithms/random_solution.h"
#include "src/algorithms/local_search_cm.h"
#include "src/algorithms/msls.h"
#include "src/algorithms/ils.h"
#include "src/algorithms/lns.h"
#include "src/algorithms/hae.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <filesystem>
#include <numeric>
#include <random>
#include <string>
#include <vector>

using NamedResult = std::pair<std::string, ExperimentResult>;

// Uruchamia algorytmy z zad. 4 na danej instancji.
//
// Zestaw:
//   - LSs_e_cm — najlepsza metoda LS z zad. 3 (LS stromy z ruchami
//     kandydackimi K=10) — uruchamiana dla porównania, jako baseline
//     (najlepsza po średniej w poprzednich iteracjach: TSPA 5854.4,
//     TSPB 17384.2).
//   - MSLS — Multiple Start Local Search: 200 iteracji LSs_e_cm
//     z różnych losowych rozwiązań startowych, zwraca najlepsze.
//   - ILS  — Iterated Local Search z małą perturbacją; warunek stopu:
//     osiągnięcie średniego czasu MSLS na tej samej instancji.
//   - LNS  — Large Neighborhood Search (Destroy-Repair, 30%) z LS w pętli.
//   - LNSa — wersja LNS bez LS w pętli (LS tylko na rozwiązanie startowe).
//
// Wszystkie metody zad. 4 uruchamiane są 20 razy.
struct RunAllOut {
    std::vector<NamedResult> results;
};

static IterStats summarize_iters(const std::vector<int>& v) {
    IterStats s;
    if (v.empty()) return s;
    s.avg     = static_cast<double>(std::accumulate(v.begin(), v.end(), 0)) / v.size();
    s.mn      = *std::min_element(v.begin(), v.end());
    s.mx      = *std::max_element(v.begin(), v.end());
    s.present = true;
    return s;
}

RunAllOut run_all(const Instance& inst, const std::string& label,
                  const ObjectiveConfig& config) {
    RunAllOut out;
    auto& results = out.results;

    auto add = [&](const std::string& name, int num_runs, auto fn) -> ExperimentResult& {
        std::cerr << "[" << label << "] " << name << " (runs=" << num_runs << ")\n";
        auto res = run_experiment(inst, fn, num_runs);
        results.emplace_back(name, std::move(res));
        return results.back().second;
    };

    constexpr int K_NEAREST = 10;
    auto nearest = compute_nearest(inst, K_NEAREST);

    // Baseline LS (najlepsza metoda z zad. 3).
    add("LSs_e_cm", 20, [&, nearest](int start) {
        std::mt19937 rng(static_cast<unsigned>(start) * 2654435761u + 3u);
        Solution init = random_solution(inst, rng);
        return local_search_cm(inst, init, nearest, config);
    });

    // MSLS — 200 iteracji LSs_e_cm na losowych startach, 20 uruchomień.
    auto msls_res = add("MSLS", 20, [&, nearest](int start) {
        std::mt19937 rng(static_cast<unsigned>(start) * 2654435761u + 17u);
        return msls(inst, nearest, rng, 200, config);
    });

    // ILS — budżet czasowy = średni czas MSLS dla tej samej instancji.
    double time_budget = msls_res.avg_time;
    std::cerr << "[" << label << "] ILS time budget = "
              << std::fixed << std::setprecision(4) << time_budget << " s\n";

    std::vector<int> ils_iters;
    auto& ils_res = add("ILS", 20, [&, nearest](int start) {
        std::mt19937 rng(static_cast<unsigned>(start) * 2654435761u + 31u);
        int iters = 0;
        Solution s = ils(inst, nearest, rng, time_budget, iters, config);
        ils_iters.push_back(iters);
        return s;
    });
    ils_res.iters = summarize_iters(ils_iters);

    // LNS — destroy 30% + repair (2-regret) + LS w pętli.
    std::vector<int> lns_iters_v;
    auto& lns_res = add("LNS", 20, [&, nearest](int start) {
        std::mt19937 rng(static_cast<unsigned>(start) * 2654435761u + 53u);
        int iters = 0;
        Solution s = lns(inst, nearest, rng, time_budget, iters,
                         /*use_local_search=*/true, /*destroy_fraction=*/0.30, config);
        lns_iters_v.push_back(iters);
        return s;
    });
    lns_res.iters = summarize_iters(lns_iters_v);

    // LNSa — wersja bez LS w pętli (LS tylko na rozwiązanie startowe).
    std::vector<int> lnsa_iters_v;
    auto& lnsa_res = add("LNSa", 20, [&, nearest](int start) {
        std::mt19937 rng(static_cast<unsigned>(start) * 2654435761u + 71u);
        int iters = 0;
        Solution s = lns(inst, nearest, rng, time_budget, iters,
                         /*use_local_search=*/false, /*destroy_fraction=*/0.30, config);
        lnsa_iters_v.push_back(iters);
        return s;
    });
    lnsa_res.iters = summarize_iters(lnsa_iters_v);

    // HAE — hybrydowy algorytm ewolucyjny (zad. 6) z trzema operatorami
    // rekombinacji; warianty 2 i 3 testujemy też bez LS po rekombinacji.
    auto run_hae = [&](const std::string& name, HaeOperator op, bool use_ls,
                       unsigned seed_off) {
        std::vector<int> iters_v;
        auto& res = add(name, 20, [&, nearest](int start) {
            std::mt19937 rng(static_cast<unsigned>(start) * 2654435761u + seed_off);
            int iters = 0;
            Solution s = hae(inst, nearest, rng, time_budget, iters, op,
                             /*use_local_search=*/use_ls, /*pop_size=*/20, config);
            iters_v.push_back(iters);
            return s;
        });
        res.iters = summarize_iters(iters_v);
    };

    run_hae("HAE1",  HaeOperator::Op1, true,  101u);
    run_hae("HAE2",  HaeOperator::Op2, true,  103u);
    run_hae("HAE2a", HaeOperator::Op2, false, 107u);
    run_hae("HAE3",  HaeOperator::Op3, true,  109u);
    run_hae("HAE3a", HaeOperator::Op3, false, 113u);

    std::cout << "=== " << label << " (" << inst.n() << " wierzchołków) ===\n";
    for (const auto& [name, res] : results) {
        print_result(name, res);
        if (res.iters.present) {
            std::cout << "             iters: avg=" << std::fixed << std::setprecision(1)
                      << res.iters.avg << "  min=" << res.iters.mn
                      << "  max=" << res.iters.mx << "\n";
        }
    }
    std::cout << "\n";

    return out;
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

    {
        std::ofstream f(dir + "/" + inst_name + "_nodes.csv");
        f << "id,x,y,profit\n";
        for (int i = 0; i < inst.n(); i++)
            f << i << "," << inst.nodes[i].x << ","
              << inst.nodes[i].y << "," << inst.nodes[i].profit << "\n";
    }

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
    f << "instance,algorithm,min_obj,max_obj,avg_obj,min_time_s,max_time_s,avg_time_s,"
         "best_start,nodes_in_best,min_iters,max_iters,avg_iters\n";
    for (const auto& [inst_name, results] : all)
        for (const auto& [name, res] : results) {
            f << inst_name << "," << name << ","
              << res.min_obj << "," << res.max_obj << ","
              << std::fixed << std::setprecision(1) << res.avg_obj << ","
              << std::setprecision(6)
              << res.min_time << "," << res.max_time << "," << res.avg_time << ","
              << res.best_start << "," << res.best.size() << ",";
            if (res.iters.present)
                f << res.iters.mn << "," << res.iters.mx << ","
                  << std::fixed << std::setprecision(1) << res.iters.avg;
            else
                f << ",,";
            f << "\n";
        }
}

int main() {
    ObjectiveConfig config = ObjectiveConfig::profit();

    Instance tspa = Instance::load("TSPA.csv");
    Instance tspb = Instance::load("TSPB.csv");

    auto out_a = run_all(tspa, "TSPA.csv", config);
    auto out_b = run_all(tspb, "TSPB.csv", config);

    save_paths("results", tspa, "TSPA", out_a.results);
    save_paths("results", tspb, "TSPB", out_b.results);
    save_checker("results", "TSPA", out_a.results);
    save_checker("results", "TSPB", out_b.results);
    save_summary("results/results.csv",
                 {{"TSPA", out_a.results}, {"TSPB", out_b.results}});

    std::cout << "Dane zapisane do katalogu results/\n";
    return 0;
}
