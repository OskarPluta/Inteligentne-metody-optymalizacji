#pragma once
#include <vector>
#include <string>

// Jeden wierzchołek: współrzędne i zysk
struct Node {
    int x, y, profit;
};

// Instancja problemu: wierzchołki + macierz odległości
struct Instance {
    std::vector<Node> nodes;
    std::vector<std::vector<int>> dist; // dist[i][j] = odległość euklidesowa zaokrąglona

    // Wczytuje instancję z pliku CSV (format: x;y;profit)
    static Instance load(const std::string& filename);

    int n() const { return static_cast<int>(nodes.size()); }
};
