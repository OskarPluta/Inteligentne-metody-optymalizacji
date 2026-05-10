#include "local_search_lm.h"
#include <algorithm>
#include <set>
#include <vector>

namespace {

enum class MoveType { Insert, Remove, EdgeSwap };

// Reprezentacja ruchu w LM identyfikowana przez wierzchołki (nie pozycje),
// dzięki czemu jest stabilna względem zmian cyklu.
//
//   Insert  : v – wierzchołek wstawiany, a, b – sąsiedzi, między których trafia
//             (saved edge a→b przed wstawieniem). Aplikowalny gdy v poza cyklem
//             oraz w cyklu istnieje krawędź {a, b}.
//   Remove  : v – wierzchołek usuwany, a, b – jego zapamiętani sąsiedzi
//             (a = prev, b = next). Aplikowalny gdy v w cyklu i jego aktualni
//             sąsiedzi to dokładnie {a, b}.
//   EdgeSwap: usuwamy krawędzie (a→b) oraz (c→d) (kierunki zapamiętane),
//             wstawiamy (a, c) i (b, d). Trzy stany aplikowalności (patrz niżej).
struct Move {
    MoveType type;
    int delta;
    int a, b, c, d;
};

struct CmpMove {
    bool operator()(const Move& x, const Move& y) const {
        if (x.delta != y.delta) return x.delta > y.delta;
        if (x.type  != y.type ) return static_cast<int>(x.type) < static_cast<int>(y.type);
        if (x.a != y.a) return x.a < y.a;
        if (x.b != y.b) return x.b < y.b;
        if (x.c != y.c) return x.c < y.c;
        return x.d < y.d;
    }
};

using LM = std::set<Move, CmpMove>;

enum class Status { Invalid, ReversedEdges, Applicable };

// --- Delty -----------------------------------------------------------------

int delta_insert(const Instance& inst, int v, int a, int b,
                 const ObjectiveConfig& cfg) {
    return cfg.node_value_sign * inst.nodes[v].profit
         - (inst.dist[a][v] + inst.dist[v][b] - inst.dist[a][b]);
}

int delta_remove(const Instance& inst, int v, int a, int b,
                 const ObjectiveConfig& cfg) {
    return -cfg.node_value_sign * inst.nodes[v].profit
         + inst.dist[a][v] + inst.dist[v][b]
         - inst.dist[a][b];
}

int delta_edge_swap(const Instance& inst, int a, int b, int c, int d) {
    // Usuwamy (a, b) i (c, d), dodajemy (a, c) i (b, d).
    return inst.dist[a][b] + inst.dist[c][d]
         - inst.dist[a][c] - inst.dist[b][d];
}

// --- Pomocnicze: utrzymanie powiązań cyklu --------------------------------

struct CycleLinks {
    std::vector<int>  next;     // next[v] = wierzchołek po v w cyklu (lub -1)
    std::vector<int>  prev;     // prev[v] = wierzchołek przed v w cyklu (lub -1)
    std::vector<char> in_cycle; // in_cycle[v] = czy v jest w cyklu
};

CycleLinks build_links(const Solution& sol, int n_total) {
    CycleLinks L;
    L.next.assign(n_total, -1);
    L.prev.assign(n_total, -1);
    L.in_cycle.assign(n_total, 0);
    int m = sol.size();
    for (int i = 0; i < m; i++) {
        int v = sol.cycle[i];
        int u = sol.cycle[(i + 1) % m];
        L.next[v] = u;
        L.prev[u] = v;
        L.in_cycle[v] = 1;
    }
    return L;
}

// Sprawdza czy krawędź {a, b} (nieskierowana) istnieje w cyklu.
bool has_edge(const CycleLinks& L, int a, int b) {
    if (!L.in_cycle[a] || !L.in_cycle[b]) return false;
    return L.next[a] == b || L.next[b] == a;
}

// --- Sprawdzenie aplikowalności --------------------------------------------

Status check_insert(const Move& m, const CycleLinks& L) {
    if (L.in_cycle[m.a] && L.next[m.a] == m.b && !L.in_cycle[m.c]) return Status::Applicable;
    if (L.in_cycle[m.b] && L.next[m.b] == m.a && !L.in_cycle[m.c]) return Status::Applicable;
    return Status::Invalid;
}

Status check_remove(const Move& m, const CycleLinks& L) {
    if (!L.in_cycle[m.a]) return Status::Invalid;
    int p = L.prev[m.a], n = L.next[m.a];
    if ((p == m.b && n == m.c) || (p == m.c && n == m.b)) return Status::Applicable;
    return Status::Invalid;
}

Status check_edge_swap(const Move& m, const CycleLinks& L) {
    // Trzy sytuacje:
    //   1. któraś z krawędzi {a,b} lub {c,d} nie istnieje  -> Invalid
    //   2. obie istnieją w tym samym kierunku co zapamiętane (lub obie odwrócone)
    //      -> Applicable
    //   3. obie istnieją, ale jedna w odwróconym kierunku  -> ReversedEdges
    if (!has_edge(L, m.a, m.b) || !has_edge(L, m.c, m.d)) return Status::Invalid;

    bool ab_fwd = (L.in_cycle[m.a] && L.next[m.a] == m.b);
    bool cd_fwd = (L.in_cycle[m.c] && L.next[m.c] == m.d);
    if (ab_fwd == cd_fwd) return Status::Applicable;
    return Status::ReversedEdges;
}

Status check_move(const Move& m, const CycleLinks& L) {
    switch (m.type) {
        case MoveType::Insert:   return check_insert(m, L);
        case MoveType::Remove:   return check_remove(m, L);
        case MoveType::EdgeSwap: return check_edge_swap(m, L);
    }
    return Status::Invalid;
}

// --- Aplikowanie ruchów na cyklu i powiązaniach ----------------------------

int find_pos(const Solution& sol, int v) {
    for (int i = 0; i < sol.size(); i++) if (sol.cycle[i] == v) return i;
    return -1;
}

void apply_insert(Solution& sol, CycleLinks& L, const Move& m) {
    // Wstawiamy v między a, b. Po check_insert wiemy, że {a, b} to krawędź.
    int a = m.a, b = m.b, v = m.c;
    int n = sol.size();
    if (L.next[a] == b) {
        int pa = find_pos(sol, a);
        sol.cycle.insert(sol.cycle.begin() + pa + 1, v);
    } else { // next[b] == a
        int pb = find_pos(sol, b);
        sol.cycle.insert(sol.cycle.begin() + pb + 1, v);
    }
    (void)n;
    L = build_links(sol, static_cast<int>(L.next.size()));
}

void apply_remove(Solution& sol, CycleLinks& L, const Move& m) {
    int p = find_pos(sol, m.a);
    sol.cycle.erase(sol.cycle.begin() + p);
    L = build_links(sol, static_cast<int>(L.next.size()));
}

void apply_edge_swap(Solution& sol, CycleLinks& L, const Move& m) {
    // Po check_edge_swap obie krawędzie istnieją w spójnym kierunku.
    // Znajdujemy pozycję wierzchołka będącego początkiem każdej krawędzi
    // (uwzględniając ewentualne globalne odwrócenie cyklu).
    int u1, v1, u2, v2;
    if (L.next[m.a] == m.b) { u1 = m.a; v1 = m.b; }
    else                    { u1 = m.b; v1 = m.a; }
    if (L.next[m.c] == m.d) { u2 = m.c; v2 = m.d; }
    else                    { u2 = m.d; v2 = m.c; }

    int i = find_pos(sol, u1);
    int j = find_pos(sol, u2);
    if (i > j) { std::swap(i, j); std::swap(u1, u2); std::swap(v1, v2); }
    // Standardowy 2-opt: odwracamy segment [i+1 .. j].
    std::reverse(sol.cycle.begin() + i + 1, sol.cycle.begin() + j + 1);
    L = build_links(sol, static_cast<int>(L.next.size()));
}

void apply_move(Solution& sol, CycleLinks& L, const Move& m) {
    switch (m.type) {
        case MoveType::Insert:   apply_insert(sol, L, m); break;
        case MoveType::Remove:   apply_remove(sol, L, m); break;
        case MoveType::EdgeSwap: apply_edge_swap(sol, L, m); break;
    }
}

// --- Generowanie nowych ruchów do LM ---------------------------------------

void try_add(LM& lm, const Move& m) {
    if (m.delta > 0) lm.insert(m);
}

void generate_all_improving(const Instance& inst, const Solution& sol,
                            const CycleLinks& L, LM& lm,
                            const ObjectiveConfig& cfg) {
    int n_total = inst.n();
    int m = sol.size();

    // Insert: dla każdego wierzchołka spoza cyklu i każdej krawędzi cyklu
    if (m >= 2 && m < n_total) {
        for (int v = 0; v < n_total; v++) {
            if (L.in_cycle[v]) continue;
            for (int a = 0; a < n_total; a++) {
                if (!L.in_cycle[a]) continue;
                int b = L.next[a];
                int d = delta_insert(inst, v, a, b, cfg);
                try_add(lm, {MoveType::Insert, d, a, b, v, 0});
            }
        }
    }

    // Remove: dla każdego wierzchołka w cyklu (m > 3 — minimalny sensowny rozmiar)
    if (m > 3) {
        for (int v = 0; v < n_total; v++) {
            if (!L.in_cycle[v]) continue;
            int p = L.prev[v], q = L.next[v];
            int d = delta_remove(inst, v, p, q, cfg);
            try_add(lm, {MoveType::Remove, d, v, p, q, 0});
        }
    }

    // EdgeSwap: dla każdej pary niesąsiednich krawędzi w cyklu.
    // Iterujemy po pozycjach (jak w wersji bez LM), żeby kontrolować degeneracje.
    if (m >= 4) {
        for (int i = 0; i < m - 1; i++) {
            for (int j = i + 2; j < m; j++) {
                if (i == 0 && j == m - 1) continue;
                int a = sol.cycle[i];
                int b = sol.cycle[(i + 1) % m];
                int c = sol.cycle[j];
                int d = sol.cycle[(j + 1) % m];

                // Wariant zgodny z bieżącym kierunkiem krawędzi.
                int d1 = delta_edge_swap(inst, a, b, c, d);
                try_add(lm, {MoveType::EdgeSwap, d1, a, b, c, d});

                // Wariant z odwróconym kierunkiem drugiej krawędzi —
                // niedostępny teraz, ale może się stać aplikowalny po
                // dowolnym ruchu odwracającym jedną z krawędzi.
                int d2 = delta_edge_swap(inst, a, b, d, c);
                try_add(lm, {MoveType::EdgeSwap, d2, a, b, d, c});
            }
        }
    }
}

} // namespace

Solution local_search_lm(const Instance& inst, Solution sol,
                         const ObjectiveConfig& config) {
    int n_total = inst.n();
    CycleLinks L = build_links(sol, n_total);

    LM lm;
    generate_all_improving(inst, sol, L, lm, config);

    while (!lm.empty()) {
        bool applied = false;
        for (auto it = lm.begin(); it != lm.end(); ) {
            Status st = check_move(*it, L);
            if (st == Status::Invalid) {
                it = lm.erase(it);
            } else if (st == Status::ReversedEdges) {
                ++it; // pomijamy, ale zostawiamy w LM
            } else { // Applicable
                Move m = *it;
                lm.erase(it);
                apply_move(sol, L, m);
                applied = true;
                break;
            }
        }
        if (!applied) {
            // Wersja alternatywna: brak aplikowalnego ruchu w LM —
            // próbujemy dolać nowe ruchy poprawiające.
            std::size_t before = lm.size();
            generate_all_improving(inst, sol, L, lm, config);
            if (lm.size() == before) break; // brak nowych ruchów — koniec
        }
    }

    return sol;
}
