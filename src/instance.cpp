#include "instance.h"
#include <fstream>
#include <sstream>
#include <cmath>
#include <stdexcept>

Instance Instance::load(const std::string& filename) {
    Instance inst;

    std::ifstream file(filename);
    if (!file)
        throw std::runtime_error("Nie można otworzyć pliku: " + filename);

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;

        std::istringstream ss(line);
        std::string token;
        Node node;

        std::getline(ss, token, ';'); node.x      = std::stoi(token);
        std::getline(ss, token, ';'); node.y      = std::stoi(token);
        std::getline(ss, token, ';'); node.profit = std::stoi(token);

        inst.nodes.push_back(node);
    }

    // Oblicz macierz odległości (euklidesowa, zaokrąglona do int)
    int n = inst.n();
    inst.dist.assign(n, std::vector<int>(n, 0));

    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            double dx = inst.nodes[i].x - inst.nodes[j].x;
            double dy = inst.nodes[i].y - inst.nodes[j].y;
            int d = static_cast<int>(std::round(std::sqrt(dx * dx + dy * dy)));
            inst.dist[i][j] = inst.dist[j][i] = d;
        }
    }

    return inst;
}
