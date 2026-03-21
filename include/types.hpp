#pragma once
#include <vector>
#include <string>
#include <cmath>
#include <numeric>
#include <algorithm>
#include <random>
#include <limits>
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <functional>
#include <cassert>
#include <iomanip>
#include <set>
#include <map>

struct Node {
    int x, y, profit;
};

struct Instance {
    std::string name;
    std::vector<Node> nodes;
    std::vector<std::vector<int>> dist; // distance matrix
    int n; // number of nodes

    void load(const std::string& filename, const std::string& inst_name = "") {
        name = inst_name.empty() ? filename : inst_name;
        nodes.clear();
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Cannot open file: " << filename << std::endl;
            return;
        }
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty()) continue;
            // Replace commas and semicolons with spaces for flexibility
            for (char& c : line) {
                if (c == ',' || c == ';') c = ' ';
            }
            std::istringstream iss(line);
            Node node;
            if (iss >> node.x >> node.y >> node.profit) {
                nodes.push_back(node);
            }
        }
        n = (int)nodes.size();
        compute_distance_matrix();
    }

    void compute_distance_matrix() {
        n = (int)nodes.size();
        dist.assign(n, std::vector<int>(n, 0));
        for (int i = 0; i < n; i++) {
            for (int j = i + 1; j < n; j++) {
                double dx = nodes[i].x - nodes[j].x;
                double dy = nodes[i].y - nodes[j].y;
                int d = (int)std::round(std::sqrt(dx * dx + dy * dy));
                dist[i][j] = d;
                dist[j][i] = d;
            }
        }
    }
};

struct Solution {
    std::vector<int> cycle; // ordered list of node indices in the cycle
    int objective;          // cached objective value

    int compute_objective(const Instance& inst) const {
        if (cycle.size() < 2) {
            objective_val = 0;
            for (int v : cycle) objective_val += inst.nodes[v].profit;
            return objective_val;
        }
        int total_profit = 0;
        int total_dist = 0;
        int sz = (int)cycle.size();
        for (int i = 0; i < sz; i++) {
            total_profit += inst.nodes[cycle[i]].profit;
            total_dist += inst.dist[cycle[i]][cycle[(i + 1) % sz]];
        }
        objective_val = total_profit - total_dist;
        return objective_val;
    }

    int get_objective(const Instance& inst) {
        return compute_objective(inst);
    }

    // Cycle length (sum of distances only)
    int cycle_length(const Instance& inst) const {
        if (cycle.size() < 2) return 0;
        int total = 0;
        int sz = (int)cycle.size();
        for (int i = 0; i < sz; i++) {
            total += inst.dist[cycle[i]][cycle[(i + 1) % sz]];
        }
        return total;
    }

    // Total profit
    int total_profit(const Instance& inst) const {
        int p = 0;
        for (int v : cycle) p += inst.nodes[v].profit;
        return p;
    }

    // Save solution to file for visualization
    void save(const std::string& filename, const Instance& inst) const {
        std::ofstream f(filename);
        f << "# objective=" << objective_val << " nodes=" << cycle.size() << "\n";
        for (int v : cycle) {
            f << inst.nodes[v].x << " " << inst.nodes[v].y << " " << v << "\n";
        }
        if (!cycle.empty()) {
            f << inst.nodes[cycle[0]].x << " " << inst.nodes[cycle[0]].y << " " << cycle[0] << "\n";
        }
    }

private:
    mutable int objective_val = 0;
};

// Statistics for experiment results
struct Stats {
    double avg;
    int min_val, max_val;
    int best_idx; // index of best run
};

inline Stats compute_stats(const std::vector<int>& values) {
    Stats s;
    s.min_val = *std::min_element(values.begin(), values.end());
    s.max_val = *std::max_element(values.begin(), values.end());
    double sum = 0;
    for (int v : values) sum += v;
    s.avg = sum / values.size();
    s.best_idx = (int)(std::max_element(values.begin(), values.end()) - values.begin());
    return s;
}
