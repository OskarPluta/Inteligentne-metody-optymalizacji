#pragma once
#include "types.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>

inline void save_solution_svg(const Solution& sol, const Instance& inst,
                               const std::string& filename, const std::string& title = "") {
    if (sol.cycle.empty()) return;

    // Find bounding box
    int min_x = std::numeric_limits<int>::max(), max_x = std::numeric_limits<int>::min();
    int min_y = std::numeric_limits<int>::max(), max_y = std::numeric_limits<int>::min();
    for (int i = 0; i < inst.n; i++) {
        min_x = std::min(min_x, inst.nodes[i].x);
        max_x = std::max(max_x, inst.nodes[i].x);
        min_y = std::min(min_y, inst.nodes[i].y);
        max_y = std::max(max_y, inst.nodes[i].y);
    }

    double margin = 40.0;
    double width = 800.0;
    double height = 600.0;
    double scale_x = (width - 2 * margin) / std::max(1, max_x - min_x);
    double scale_y = (height - 2 * margin) / std::max(1, max_y - min_y);
    double scale = std::min(scale_x, scale_y);

    auto tx = [&](int x) { return margin + (x - min_x) * scale; };
    auto ty = [&](int y) { return height - margin - (y - min_y) * scale; }; // flip y

    std::set<int> in_cycle(sol.cycle.begin(), sol.cycle.end());

    std::ofstream f(filename);
    f << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    f << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" << (int)width
      << "\" height=\"" << (int)height + 30 << "\" viewBox=\"0 0 " << (int)width
      << " " << (int)height + 30 << "\">\n";
    f << "<rect width=\"100%\" height=\"100%\" fill=\"white\"/>\n";

    if (!title.empty()) {
        f << "<text x=\"" << width / 2 << "\" y=\"20\" text-anchor=\"middle\" "
          << "font-family=\"sans-serif\" font-size=\"14\" font-weight=\"bold\">"
          << title << "</text>\n";
    }

    // Draw non-selected nodes (gray, small)
    for (int i = 0; i < inst.n; i++) {
        if (in_cycle.count(i)) continue;
        f << "<circle cx=\"" << tx(inst.nodes[i].x) << "\" cy=\"" << ty(inst.nodes[i].y) + 25
          << "\" r=\"3\" fill=\"#cccccc\" />\n";
    }

    // Draw cycle edges
    int sz = (int)sol.cycle.size();
    for (int i = 0; i < sz; i++) {
        int a = sol.cycle[i];
        int b = sol.cycle[(i + 1) % sz];
        f << "<line x1=\"" << tx(inst.nodes[a].x) << "\" y1=\"" << ty(inst.nodes[a].y) + 25
          << "\" x2=\"" << tx(inst.nodes[b].x) << "\" y2=\"" << ty(inst.nodes[b].y) + 25
          << "\" stroke=\"#2563eb\" stroke-width=\"1.5\" />\n";
    }

    // Draw selected nodes (blue, larger) with index labels
    for (int v : sol.cycle) {
        double cx = tx(inst.nodes[v].x);
        double cy = ty(inst.nodes[v].y) + 25;
        f << "<circle cx=\"" << cx << "\" cy=\"" << cy
          << "\" r=\"5\" fill=\"#2563eb\" stroke=\"white\" stroke-width=\"1\" />\n";
        f << "<text x=\"" << cx + 7 << "\" y=\"" << cy + 4
          << "\" font-family=\"sans-serif\" font-size=\"9\" fill=\"#1e3a5f\">"
          << v << "</text>\n";
    }

    f << "</svg>\n";
}
