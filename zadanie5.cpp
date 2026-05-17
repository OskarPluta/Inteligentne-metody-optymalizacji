#include "src/instance.h"
#include "src/solution.h"
#include "src/objective.h"
#include "src/algorithms/random_solution.h"
#include "src/algorithms/local_search.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <random>
#include <string>
#include <vector>

// Zadanie 5 — testy globalnej wypukłosci.
// Generuje 1000 losowych optimow lokalnych (greedy LS, EdgeSwap) dla TSPA i TSPB.
// Zapisuje cykl (kolejnosc wierzcholkow) i wartosc funkcji celu kazdego optimum.

static void generate(const Instance& inst, const std::string& inst_name,
                     int num, unsigned base_seed,
                     const ObjectiveConfig& config) {
    namespace fs = std::filesystem;
    fs::create_directories("results");
    std::string path = "results/" + inst_name + "_local_optima.csv";
    std::ofstream f(path);
    f << "id,objective,size,cycle\n";

    for (int i = 0; i < num; i++) {
        std::mt19937 rng(base_seed + static_cast<unsigned>(i) * 2654435761u);
        Solution init = random_solution(inst, rng);
        Solution s = local_search(inst, init, LSMode::Greedy,
                                  LSNeighborhood::EdgeSwap, rng, config);
        int obj = s.objective(inst, config);
        f << i << "," << obj << "," << s.size() << ",";
        for (int j = 0; j < s.size(); j++) {
            if (j) f << ' ';
            f << s.cycle[j];
        }
        f << "\n";
        if ((i + 1) % 50 == 0)
            std::cerr << "[" << inst_name << "] " << (i + 1) << "/" << num << "\n";
    }
    std::cerr << "Zapisano " << path << "\n";
}

int main() {
    ObjectiveConfig config = ObjectiveConfig::profit();

    Instance tspa = Instance::load("TSPA.csv");
    Instance tspb = Instance::load("TSPB.csv");

    generate(tspa, "TSPA", 1000, 0xA5A5u, config);
    generate(tspb, "TSPB", 1000, 0xB7B7u, config);

    std::cout << "Gotowe — wyniki w results/TSPA_local_optima.csv i results/TSPB_local_optima.csv\n";
    return 0;
}
