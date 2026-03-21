// projekt_tsp_full.cpp
// Wymagane nagłówki:
#include "rapidcsv.h"
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <random>
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <functional>
#include <numeric>
#include <cassert>

// -----------------------------------------------------------------------------
// Legenda spełnianych wymagań (skrót):
// [W1] Wczytywanie instancji i obliczanie macierzy odległości.
// [W2] Losowe rozwiązanie (losujemy k i permutujemy wierzchołki).
// [W3] Nearest Neighbor (NNa i NN z profitem) — faza I buduje Hamiltona.
// [W4] Greedy Cycle (GCa i GC z profitem) — faza I buduje Hamiltona.
// [W5] Regret-2 heurystyka.
// [W6] Weighted Regret-2.
// [W7] Efektywne obliczanie delty (insertion/removal).
// [W8] Faza II: prune — usuwanie wierzchołków poprawiających objective.
// [W9] Eksperymenty: 200 uruchomień, różne starty, zbieranie statystyk.
// [W10] Opcjonalna heurystyka: Seed-10% + rozbudowa (dodana).
// -----------------------------------------------------------------------------

// -----------------------------
// csv_to_2d_double_ptr
// Pseudokod:
//  otwórz plik CSV przy użyciu rapidcsv
//  dla każdej linii:
//    pobierz wiersz jako vector<string>
//    dla każdego tokenu: trim, stod (jeśli nie parsuje -> warn i 0.0)
//    dodaj row do out
//  walidacja: out niepuste, każdy row ma >=2 kolumny
// Zgodność: [W1]
// -----------------------------



static std::vector<std::vector<double>>* csv_to_2d_double_ptr(const std::string& filepath, char sep = ';') {
    try {
        rapidcsv::SeparatorParams sp;
        sp.mSeparator = sep;
        rapidcsv::Document doc(filepath, rapidcsv::LabelParams(-1, -1), sp);
        size_t rows = doc.GetRowCount();
        auto out = new std::vector<std::vector<double>>();
        out->reserve(rows);
        for (size_t r = 0; r < rows; ++r) {
            std::vector<std::string> rowStr = doc.GetRow<std::string>(r);
            std::vector<double> row;
            row.reserve(rowStr.size());
            for (auto& cell : rowStr) {
                size_t a = 0;
                while (a < cell.size() && isspace((unsigned char)cell[a])) ++a;
                size_t b = cell.size();
                while (b > a && isspace((unsigned char)cell[b - 1])) --b;
                std::string t = cell.substr(a, b - a);
                if (t.empty()) {
                    row.push_back(0.0);
                }
                else {
                    try {
                        row.push_back(std::stod(t));
                    }
                    catch (...) {
                        std::cerr << "Warning: cannot parse token '" << t << "' at row " << r << "\n";
                        row.push_back(0.0);
                    }
                }
            }
            out->push_back(std::move(row));
        }
        if (out->empty()) throw std::runtime_error("CSV empty or unreadable");
        for (size_t i = 0; i < out->size(); ++i) {
            if ((*out)[i].size() < 2) throw std::runtime_error("CSV row has less than 2 columns (X,Y) at row " + std::to_string(i));
        }
        return out;
    }
    catch (const std::exception& e) {
        std::cerr << "csv_to_2d_double_ptr: blad wczytywania pliku: " << e.what() << "\n";
        return nullptr;
    }
}

// -----------------------------
// compute_distance_matrix
// Pseudokod:
//  n = data.size(); alokuj n x n macierz 0
//  dla i<j: d = round(sqrt((xi-xj)^2 + (yi-yj)^2)); dist[i][j]=dist[j][i]=d
// Zgodność: [W1]
// -----------------------------

/*
    Jezeli wektor to nullptr
        throw_exception("pointer do danych jest pusty")
        n = pobierz_rozmiar(dane)
        dist = vector(n,n)
        dla kazdego wiersza :
            jezeli liczba_kolumn<2:
                throw_exception("liczba kolumn jest nie zachowana")
        dla kazdego wiercholka od i do n
            dla kazdego wiercholka od i+1 do n:
                dystans = [(xi-xj)^2 + (yi-yj)^2]^0.5
                dystans = zaokrogl(dystans)
                dist[i][j] = dystans
                dist[j][i] = dystans
        zwroc dist

*/
static std::vector<std::vector<double>>* compute_distance_matrix(const std::vector<std::vector<double>>* data) {
    if (!data) throw std::invalid_argument("compute_distance_matrix: data null");
    size_t n = data->size();
    auto dist = new std::vector<std::vector<double>>(n, std::vector<double>(n, 0.0));
    for (size_t i = 0; i < n; ++i) {
        if ((*data)[i].size() < 2) throw std::runtime_error("compute_distance_matrix: row has less than 2 columns");
    }
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = i + 1; j < n; ++j) {
            double xi = (*data)[i][0];
            double yi = (*data)[i][1];
            double xj = (*data)[j][0];
            double yj = (*data)[j][1];
            double dx = xi - xj;
            double dy = yi - yj;
            double d = std::sqrt(dx * dx + dy * dy);
            d = std::round(d); // matematyczne zaokrąglenie
            (*dist)[i][j] = d;
            (*dist)[j][i] = d;
        }
    }
    return dist;
}

// -----------------------------
// tour_length
// Pseudokod:
//  jeśli tour pusty -> 0
//  sum = 0; dla i: sum += D[tour[i]][tour[(i+1)%m]]
// Zgodność: [W7]
// -----------------------------

/*
    suma_wycieczki = 0
    rozmiar wycieczki = rozmiar(tour)
    dla kazdego i od 0 do rozmianr_wycieczki-1:
        wiercholek_A = tour[i]
        wierzochlek_B = tour[(i+1) modoulo m] # zeby potem dodac ostatni wierchole z pierwszym
        suma_wycieczki = suma_wycieczki + macierz_odleglosci(wiercholek_A,wiercholek_B)
    zwroc suma_wycieczki

*/
static double tour_length(const std::vector<int>& tour, const std::vector<std::vector<double>>* D) {
    if (!D) throw std::invalid_argument("tour_length: D null");
    if (tour.empty()) return 0.0;
    double sum = 0.0;
    size_t m = tour.size();
    for (size_t i = 0; i < m; ++i) {
        int a = tour[i];
        int b = tour[(i + 1) % m];
        sum += (*D)[a][b];
    }
    return sum;
}

// -----------------------------
// tour_profit
// Pseudokod:
//  sum profitów z data[v][2] jeśli istnieje
// Zgodność: [W7]
// -----------------------------

/*
    suma_profitow=0
    dla kazdego miasta w sciezce:
        suma_profitow+=profit[miasto]
    zwroc profit
    

*/
static double tour_profit(const std::vector<int>& tour, const std::vector<std::vector<double>>* data) {
    if (!data) throw std::invalid_argument("tour_profit: data null");
    double sum = 0.0;
    for (int v : tour) {
        if (v < 0 || static_cast<size_t>(v) >= data->size()) continue;
        const auto& row = (*data)[v];
        double p = (row.size() > 2) ? row[2] : 0.0;
        sum += p;
    }
    return sum;
}

// -----------------------------
// objective_of_tour
// Pseudokod:
//  return tour_profit - tour_length
// -----------------------------

/*
    
    zwroc Suma(profit_wycieczki(tour)) - Suma(dlogosc_wycieczki(tour))
*/
static double objective_of_tour(const std::vector<int>& tour, const std::vector<std::vector<double>>* D, const std::vector<std::vector<double>>* data) {
    return tour_profit(tour, data) - tour_length(tour, D);
}

// -----------------------------
// insertion_cost_delta
// Pseudokod:
//  return D[a][v] + D[v][b] - D[a][b]
// Zgodność: [W7]
// -----------------------------
static double insertion_cost_delta(const std::vector<std::vector<double>>* D, int a, int v, int b) {
    return (*D)[a][v] + (*D)[v][b] - (*D)[a][b];
}

// -----------------------------
// removal_objective_delta
// Pseudokod:
//  profitDelta = -profit[v]
//  insertionDelta = D[a][v] + D[v][b] - D[a][b]
//  lengthDelta = -insertionDelta
//  return profitDelta - lengthDelta
// Zgodność: [W7], [W8]
// -----------------------------
static double removal_objective_delta(const std::vector<std::vector<double>>* D, const std::vector<double>& profit, int a, int v, int b) {
    double profitDelta = -profit[v];
    double insertionDelta = (*D)[a][v] + (*D)[v][b] - (*D)[a][b];
    double lengthDelta = -insertionDelta;
    return profitDelta - lengthDelta;
}

// -----------------------------
// random_solution
// Pseudokod:
//  n = data.size(); k = losuj z [1..n]; all = [0..n-1]; shuffle; return first k
// Zgodność: [W2]
// -----------------------------
/*



    n= ilosc_wierzhcolkow(dane)
        jezeli liczba wierzchokow to 0:
            zwroc pusty vektor
        k = losuj_liczbe_wierzcholkow(3,n)
        all = vektor()
        all = liczby_z_przedzialu(0,n-1)
        all = wymieszaj(all)
        trasa = all(poczatek,poczatek+k)
        zwroc trasa



*/

static std::vector<int>* random_solution(const std::vector<std::vector<double>>* data, std::mt19937& rng) {
    size_t n = data->size();
    if (n == 0) return new std::vector<int>();
    std::uniform_int_distribution<int> distK(3, static_cast<int>(n));
    int k = distK(rng);
    std::vector<int> all(n);
    std::iota(all.begin(), all.end(), 0);

    std::shuffle(all.begin(), all.end(), rng);
    auto tour = new std::vector<int>(all.begin(), all.begin() + k);
    return tour;
}

// -----------------------------
// nearest_neighbor
// Pseudokod:
//  used=false[n]; tour=[start]; while size<n: wybierz v not used minimalizujący score
//  score = D[last][v] (NNa) lub D[last][v] - profitWeight*profit[v] (NN z profitem)
// Zgodność: [W3]
// -----------------------------

/*
    used = vektor(N_elementow,0)
    trasa = Vector()
    trasa->dodaj(elemnt_startowy)
    dopoki trasa < wszytskie elementy
        wczytaj ostatni element
            dla kazdego wierzcholka w zbiorze :
                jezeli wiercholek juz zostal uzyty:
                    idz do nastepnej iteracji
                wynik = odleglosc(ostatni_element,wierzcholek)
                jezeli uwzgledniamy profit:
                    p=profit(wiercholek)
                    wynik = wynik - waga_profitu*p
                jezeli wynik <obecny:
                    obecny_wynik =wynik
                    zapamietaj wierzholek jako best: best = wiercholek
            jezeli nie znaleziono lepszego wierzcholka konczymy petle
            trasa->dodaj(best)
            usded->dodaj(best)
    zwroc trasa
                



*/
static std::vector<int>* nearest_neighbor(const std::vector<std::vector<double>>* D,
    const std::vector<std::vector<double>>* data,
    bool considerProfit,
    double profitWeight,
    int start) {
    size_t n = data->size();
    if (n == 0) return new std::vector<int>();
    std::vector<char> used(n, 0);
    auto tour = new std::vector<int>();
    int s = (start >= 0 && static_cast<size_t>(start) < n) ? start : 0;
    tour->push_back(s);
    used[s] = 1;
    while (tour->size() < n) {
        int last = tour->back();
        int best = -1;
        double bestScore = std::numeric_limits<double>::infinity();
        for (size_t v = 0; v < n; ++v) {
            if (used[v]) continue;
            double score = (*D)[last][v];
            if (considerProfit) {
                double p = ((*data)[v].size() > 2) ? (*data)[v][2] : 0.0;
                score = score - profitWeight * p;
            }
            if (score < bestScore) {
                bestScore = score;
                best = static_cast<int>(v);
            }
        }
        if (best == -1) break;
        tour->push_back(best);
        used[best] = 1;
    }
    return tour;
}

// -----------------------------
// greedy_cycle_build
// Pseudokod:
//  s=start; nearest = argmin D[s][v]; tour=[s,nearest]
//  while tour.size()<n:
//    dla v not in tour: policz bestDeltaForV i bestPosForV
//    score = bestDeltaForV (lub - profitWeight*profit[v])
//    wybierz v minimalizujące score; wstaw
// Zgodność: [W4]
// -----------------------------

/*
    wczytaj wiercholek startowy
    # tworzmy cykl A->B->A
    w_out=-1
    dla kazdego wiercholka jako w
            jezelli w == wierzcholek_startowy:
                kontynuuj obieg petli
            jezeli dystans[wiercholek_startowy][w]<obecnie_najlepszy_dystans
                najlepszy_dystans= dystans[wiercholek_startowy][w]
                w_out =w
    czy_w_trasie=Vektor(N,0)
    trasa = Vektor();
    trasa->dodaj(wiercholek_startowy)
    czy_w_trasie[wiercholek_startowy]=1
    trasa->dodaj(w_out)
    czy_w_trasie->dodaj(w_out)

    dopuki dlugosc(trasa)<n:
        dla kazdego wiercholka jako v :
            jezeli v w trasie:
                przejdz do nastepenj iteracji

        
    
 




*/
static std::vector<int>* greedy_cycle_build(const std::vector<std::vector<double>>* D,
    const std::vector<std::vector<double>>* data,
    bool considerProfit,
    double profitWeight,
    int start) {
    size_t n = data->size();
    if (n == 0) return new std::vector<int>();
    int s = (start >= 0 && static_cast<size_t>(start) < n) ? start : 0;
    int nearest = -1;
    double bestd = std::numeric_limits<double>::infinity();
    for (size_t v = 0; v < n; ++v) {
        if (static_cast<int>(v) == s) continue;
        if ((*D)[s][v] < bestd) { bestd = (*D)[s][v]; nearest = static_cast<int>(v); }
    }
    std::vector<char> inTour(n, 0);
    auto tour = new std::vector<int>();
    tour->push_back(s);
    inTour[s] = 1;
    if (nearest != -1) {
        tour->push_back(nearest);
        inTour[nearest] = 1;
    }
    while (tour->size() < n) {
        int bestV = -1;
        int bestPos = -1;
        double bestScore = std::numeric_limits<double>::infinity();
        double bestDeltaTie = std::numeric_limits<double>::infinity();
        for (size_t v = 0; v < n; ++v) {
            if (inTour[v]) continue;
            double bestDeltaForV = std::numeric_limits<double>::infinity();
            int bestPosForV = -1;
            size_t m = tour->size();
            for (size_t pos = 0; pos < m; ++pos) {
                int a = (*tour)[pos];
                int b = (*tour)[(pos + 1) % m];
                double delta = insertion_cost_delta(D, a, static_cast<int>(v), b);
                if (delta < bestDeltaForV) {
                    bestDeltaForV = delta;
                    bestPosForV = static_cast<int>(pos);
                }
            }
            double score = bestDeltaForV;
            if (considerProfit) {
                double p = ((*data)[v].size() > 2) ? (*data)[v][2] : 0.0;
                score = bestDeltaForV - profitWeight * p;
            }
            if (score < bestScore || (score == bestScore && bestDeltaForV < bestDeltaTie)) {
                bestScore = score;
                bestV = static_cast<int>(v);
                bestPos = bestPosForV;
                bestDeltaTie = bestDeltaForV;
            }
        }
        if (bestV == -1) break;
        tour->insert(tour->begin() + bestPos + 1, bestV);
        inTour[bestV] = 1;
    }
    return tour;
}

// -----------------------------
// regret_2
// Pseudokod:
//  inicjalizacja [s, nearest]
//  while tour.size()<n:
//    dla v not in tour: policz bestDelta i secondDelta (przegląd pozycji)
//    regret = secondDelta - bestDelta
//    wybierz v z maks regret (tie-breaker: mniejszy bestDelta)
//    wstaw v w najlepszą pozycję
// Zgodność: [W5]
// -----------------------------
static std::vector<int>* regret_2(const std::vector<std::vector<double>>* D,
    const std::vector<std::vector<double>>* data,
    int start) {
    size_t n = data->size();
    if (n == 0) return new std::vector<int>();
    int s = (start >= 0 && static_cast<size_t>(start) < n) ? start : 0;
    int nearest = -1;
    double bestd = std::numeric_limits<double>::infinity();
    for (size_t v = 0; v < n; ++v) {
        if (static_cast<int>(v) == s) continue;
        if ((*D)[s][v] < bestd) { bestd = (*D)[s][v]; nearest = static_cast<int>(v); }
    }
    std::vector<char> inTour(n, 0);
    auto tour = new std::vector<int>();
    tour->push_back(s);
    inTour[s] = 1;
    if (nearest != -1) { tour->push_back(nearest); inTour[nearest] = 1; }
    while (tour->size() < n) {
        int bestV = -1;
        int bestPos = -1;
        double bestRegret = -std::numeric_limits<double>::infinity();
        double tieBestDelta = std::numeric_limits<double>::infinity();
        for (size_t v = 0; v < n; ++v) {
            if (inTour[v]) continue;
            double bestDelta = std::numeric_limits<double>::infinity();
            double secondDelta = std::numeric_limits<double>::infinity();
            int bestPosForV = -1;
            size_t m = tour->size();
            for (size_t pos = 0; pos < m; ++pos) {
                int a = (*tour)[pos];
                int b = (*tour)[(pos + 1) % m];
                double delta = insertion_cost_delta(D, a, static_cast<int>(v), b);
                if (delta < bestDelta) {
                    secondDelta = bestDelta;
                    bestDelta = delta;
                    bestPosForV = static_cast<int>(pos);
                }
                else if (delta < secondDelta) {
                    secondDelta = delta;
                }
            }
            if (secondDelta == std::numeric_limits<double>::infinity()) secondDelta = bestDelta;
            double regret = secondDelta - bestDelta;
            if (regret > bestRegret || (regret == bestRegret && bestDelta < tieBestDelta)) {
                bestRegret = regret;
                bestV = static_cast<int>(v);
                bestPos = bestPosForV;
                tieBestDelta = bestDelta;
            }
        }
        if (bestV == -1) break;
        tour->insert(tour->begin() + bestPos + 1, bestV);
        inTour[bestV] = 1;
    }
    return tour;
}

// -----------------------------
// weighted_regret_2
// Pseudokod:
//  jak regret_2, ale score = weightRegret * regret - weightCost * bestDelta
// Zgodność: [W6]
// -----------------------------
static std::vector<int>* weighted_regret_2(const std::vector<std::vector<double>>* D,
    const std::vector<std::vector<double>>* data,
    double weightRegret = 1.0,
    double weightCost = 1.0,
    int start = 0) {

    size_t n = data->size();
    if (n == 0) return new std::vector<int>();

    // Inicjalizacja cyklu (2 pierwsze punkty) - tu jest ok
    int s = (start >= 0 && static_cast<size_t>(start) < n) ? start : 0;
    int nearest = -1;
    double bestd = std::numeric_limits<double>::infinity();
    for (size_t v = 0; v < n; ++v) {
        if (static_cast<int>(v) == s) continue;
        if ((*D)[s][v] < bestd) { bestd = (*D)[s][v]; nearest = static_cast<int>(v); }
    }

    std::vector<char> inTour(n, 0);
    auto tour = new std::vector<int>();
    tour->push_back(s);
    inTour[s] = 1;
    if (nearest != -1) { tour->push_back(nearest); inTour[nearest] = 1; }

    while (tour->size() < n) {
        int bestV = -1;
        int bestPos = -1;
        // POPRAWKA: Szukamy MAKSYMALNEGO score, więc startujemy od -infinity
        double bestScore = -std::numeric_limits<double>::infinity();
        double tieBestDelta = std::numeric_limits<double>::infinity();

        for (size_t v = 0; v < n; ++v) {
            if (inTour[v]) continue;

            // POPRAWKA: Szukamy MINIMALNYCH kosztów wstawienia, więc tu musi być +infinity
            double bestDelta = std::numeric_limits<double>::infinity();
            double secondDelta = std::numeric_limits<double>::infinity();
            int bestPosForV = -1;

            size_t m = tour->size();
            for (size_t pos = 0; pos < m; ++pos) {
                int a = (*tour)[pos];
                int b = (*tour)[(pos + 1) % m];
                double delta = insertion_cost_delta(D, a, static_cast<int>(v), b);

                if (delta < bestDelta) {
                    secondDelta = bestDelta;
                    bestDelta = delta;
                    bestPosForV = static_cast<int>(pos);
                }
                else if (delta < secondDelta) {
                    secondDelta = delta;
                }
            }

            // Jeśli jest tylko jedna opcja wstawienia, żal = 0 (lub można dać dużą karę)
            if (secondDelta == std::numeric_limits<double>::infinity()) secondDelta = bestDelta;

            double regret = secondDelta - bestDelta;
            // POPRAWKA: Ważony żal. Promujemy duży żal i mały koszt (dlatego minus bestDelta)
            double score = weightRegret * regret - weightCost * bestDelta;

            // POPRAWKA: Szukamy MAKSIMUM score (największy żal)
            if (score > bestScore || (score == bestScore && bestDelta < tieBestDelta)) {
                bestScore = score;
                bestV = static_cast<int>(v);
                bestPos = bestPosForV;
                tieBestDelta = bestDelta;
            }
        }

        if (bestV == -1) break;
        tour->insert(tour->begin() + bestPos + 1, bestV);
        inTour[bestV] = 1;
    }
    return tour;
}

// -----------------------------
// Seed-10% + Greedy expand (opcjonalna heurystyka)
// Pseudokod (Wariant A - rozbudowa do Hamiltona):
//  k = max(1, round(seedFraction * n))
//  S = losowy podzbiór k wierzchołków
//  tour = greedy_cycle_build ograniczony do S (lub NN na S)
//  dla v in V \ S: wstaw v w najlepsze miejsce minimalizujące insertion_cost_delta
//  lengthPhaseI = tour_length(tour)  // zapis przed prune
//  prune_remove_improving(tour)
// Zgodność: [W10] (opcjonalna heurystyka), używa [W7] i może korzystać z [W8]
// -----------------------------
static std::vector<int>* seed10_percent_expand(const std::vector<std::vector<double>>* D,
    const std::vector<std::vector<double>>* data,
    double seedFraction,
    std::mt19937& rng) {
    size_t n = data->size();
    if (n == 0) return new std::vector<int>();
    int k = std::max(1, static_cast<int>(std::round(seedFraction * n)));
    // losowy podzbiór S
    std::vector<int> all(n);
    std::iota(all.begin(), all.end(), 0);
    std::shuffle(all.begin(), all.end(), rng);
    std::vector<char> inTour(n, 0);
    std::vector<int> tourVec;
    tourVec.reserve(n);
    for (int i = 0; i < k; ++i) {
        tourVec.push_back(all[i]);
        inTour[all[i]] = 1;
    }
    // Zbuduj cykl na S używając greedy_cycle_build ograniczonego do S:
    // prosty sposób: posortuj S według greedy: wybierz start = S[0], znajdź nearest w S\{start}, potem wstawiaj pozostałe z S minimalnym insertion
    // Implementacja uproszczona: użyj greedy_cycle_build na całym D, ale zatrzymaj po wstawieniu wszystkich z S (efekt: musimy implementować ograniczenie)
    // Dla prostoty: zbudujemy cykl na S ręcznie:
    // start = tourVec[0]; znajdz nearest w S; utwórz [s, nearest]; potem wstawiaj pozostałe z S minimalnym insertion
    int s = tourVec[0];
    int nearest = -1;
    double bestd = std::numeric_limits<double>::infinity();
    for (int i = 1; i < k; ++i) {
        int v = tourVec[i];
        if ((*D)[s][v] < bestd) { bestd = (*D)[s][v]; nearest = v; }
    }
    std::vector<int> tour;
    tour.push_back(s);
    std::vector<char> inS(n, 0);
    inS[s] = 1;
    if (nearest != -1) { tour.push_back(nearest); inS[nearest] = 1; }
    // wstaw pozostałe z S
    for (int i = 1; i < k; ++i) {
        int v = tourVec[i];
        if (inS[v]) continue;
        // znajdz najlepsza pozycje
        double bestDelta = std::numeric_limits<double>::infinity();
        int bestPos = -1;
        size_t m = tour.size();
        for (size_t pos = 0; pos < m; ++pos) {
            int a = tour[pos];
            int b = tour[(pos + 1) % m];
            double delta = insertion_cost_delta(D, a, v, b);
            if (delta < bestDelta) { bestDelta = delta; bestPos = static_cast<int>(pos); }
        }
        tour.insert(tour.begin() + bestPos + 1, v);
        inS[v] = 1;
    }
    // teraz rozbuduj o reszte wierzcholkow V \ S
    for (size_t v = 0; v < n; ++v) {
        if (inS[v]) continue;
        // najlepsza pozycja w obecnym tour
        double bestDelta = std::numeric_limits<double>::infinity();
        int bestPos = -1;
        size_t m = tour.size();
        for (size_t pos = 0; pos < m; ++pos) {
            int a = tour[pos];
            int b = tour[(pos + 1) % m];
            double delta = insertion_cost_delta(D, a, static_cast<int>(v), b);
            if (delta < bestDelta) { bestDelta = delta; bestPos = static_cast<int>(pos); }
        }
        tour.insert(tour.begin() + bestPos + 1, static_cast<int>(v));
        inS[v] = 1;
    }
    return new std::vector<int>(tour.begin(), tour.end());
}

// -----------------------------
// prune_remove_improving
// Pseudokod:
//  while true:
//    bestDelta = 0; bestIdx = -1
//    dla idx: a=prev, v=tour[idx], b=next; delta = removal_objective_delta
//    jeśli delta > bestDelta: zapamietaj
//    jeśli bestIdx == -1: break
//    usuń tour[bestIdx]
// Zgodność: [W8]
// -----------------------------
static bool prune_remove_improving(std::vector<int>& tour,
    const std::vector<std::vector<double>>* D,
    const std::vector<double>& profit) {
    if (tour.size() < 2) return false;
    bool anyRemoved = false;
    while (true) {
        double bestDelta = 0.0;
        int bestIdx = -1;
        size_t m = tour.size();
        for (size_t idx = 0; idx < m; ++idx) {
            int v = tour[idx];
            int a = tour[(idx + m - 1) % m];
            int b = tour[(idx + 1) % m];
            double delta = removal_objective_delta(D, profit, a, v, b);
            if (delta > bestDelta) {
                bestDelta = delta;
                bestIdx = static_cast<int>(idx);
            }
        }
        if (bestIdx == -1) break;
        tour.erase(tour.begin() + bestIdx);
        anyRemoved = true;
        if (tour.size() < 2) break;
    }
    return anyRemoved;
}

// -----------------------------
// ExperimentResult (rozszerzony o lengthPhaseI i visitedAfter)
// Zgodność: [W9]
// -----------------------------
struct ExperimentResult {
    std::vector<double> allObjs;        // objective po fazie II (po prune)
    std::vector<double> lengthPhaseI;   // długość cyklu po fazie I (przed prune)
    std::vector<int> visitedAfter;      // liczba odwiedzonych wierzchołków po fazie II
    double mean = 0.0;
    double stddev = 0.0;
    double minv = std::numeric_limits<double>::infinity();
    double maxv = -std::numeric_limits<double>::infinity();
    double bestObj = -std::numeric_limits<double>::infinity();
    std::vector<int>* bestTour = nullptr;
};

// -----------------------------
// run_experiments_collect_stats
// Pseudokod:
//  dla run in 0..runs-1:
//    start = run % n (lub losowy start jeśli chcesz)
//    tour = heuristic(start)
//    length_before_prune = tour_length(tour)
//    res.lengthPhaseI.push_back(length_before_prune)
//    if doPrune: prune_remove_improving(tour)
//    obj_after = objective_of_tour(tour)
//    res.allObjs.push_back(obj_after); res.visitedAfter.push_back(tour.size())
//  oblicz statystyki
// Zgodność: [W9]
// -----------------------------
using HeuristicFn = std::function<std::vector<int>* (int start)>;

static ExperimentResult run_experiments_collect_stats(const std::vector<std::vector<double>>* data,
    const std::vector<std::vector<double>>* D,
    HeuristicFn heuristic,
    int runs,
    bool doPrune,
    const std::vector<double>& profit,
    bool deterministicStarts = true) {
    ExperimentResult res;
    size_t n = data->size();
    res.allObjs.reserve(runs);
    res.lengthPhaseI.reserve(runs);
    res.visitedAfter.reserve(runs);
    for (int run = 0; run < runs; ++run) {
        int start;
        if (deterministicStarts) start = run % static_cast<int>(n);
        else {
            static std::mt19937 rng_local(12345);
            std::uniform_int_distribution<int> ds(0, static_cast<int>(n) - 1);
            start = ds(rng_local);
        }
        std::vector<int>* tour = heuristic(start);
        if (!tour) {
            res.allObjs.push_back(-std::numeric_limits<double>::infinity());
            res.lengthPhaseI.push_back(-std::numeric_limits<double>::infinity());
            res.visitedAfter.push_back(0);
            continue;
        }

        // Zapis długości po fazie I (przed prune)
        double length_before_prune = tour_length(*tour, D);
        res.lengthPhaseI.push_back(length_before_prune);

        // Faza II: prune
        if (doPrune) {
            prune_remove_improving(*tour, D, profit);
        }
        double obj = objective_of_tour(*tour, D, data);
        res.allObjs.push_back(obj);
        res.visitedAfter.push_back(static_cast<int>(tour->size()));
        if (obj < res.minv) res.minv = obj;
        if (obj > res.maxv) res.maxv = obj;
        if (obj > res.bestObj) {
            if (res.bestTour) delete res.bestTour;
            res.bestTour = tour;
            res.bestObj = obj;
        }
        else {
            delete tour;
        }
    }
    // statystyki dla allObjs
    double sum = 0.0;
    int validCount = 0;
    for (double v : res.allObjs) {
        if (std::isfinite(v)) { sum += v; ++validCount; }
    }
    res.mean = (validCount > 0) ? (sum / validCount) : 0.0;
    double var = 0.0;
    for (double v : res.allObjs) {
        if (std::isfinite(v)) var += (v - res.mean) * (v - res.mean);
    }
    res.stddev = (validCount > 1) ? std::sqrt(var / (validCount - 1)) : 0.0;
    if (res.minv == std::numeric_limits<double>::infinity()) res.minv = 0.0;
    if (res.maxv == -std::numeric_limits<double>::infinity()) res.maxv = 0.0;
    return res;
}

// -----------------------------
// print helpers
// -----------------------------
static void print_tour_brief(const std::vector<int>* tour) {
    if (!tour) { std::cout << "(null)\n"; return; }
    for (size_t i = 0; i < tour->size(); ++i) {
        if (i) std::cout << ' ';
        std::cout << (*tour)[i];
    }
    std::cout << '\n';
}

static void print_stats_and_best(const std::string& name, const ExperimentResult& res, const std::vector<std::vector<double>>* data, const std::vector<std::vector<double>>* D) {
    std::cout << "Running heuristic: " << name << " (" << res.allObjs.size() << " runs)\n";
    std::cout << "  mean:   " << res.mean << "\n";
    std::cout << "  stddev: " << res.stddev << "\n";
    std::cout << "  min:    " << res.minv << "\n";
    std::cout << "  max:    " << res.maxv << "\n";
    std::cout << "  best:   " << res.bestObj << "\n";

    if (!res.lengthPhaseI.empty()) {
        double sum = 0.0; int cnt = 0;
        double minv = std::numeric_limits<double>::infinity(), maxv = -std::numeric_limits<double>::infinity();
        for (double x : res.lengthPhaseI) if (std::isfinite(x)) { sum += x; ++cnt; minv = std::min(minv, x); maxv = std::max(maxv, x); }
        double mean = (cnt > 0) ? sum / cnt : 0.0;
        if (minv == std::numeric_limits<double>::infinity()) { minv = 0.0; maxv = 0.0; }
        std::cout << "  length after Phase I (mean (min - max)): " << mean << " (" << minv << " - " << maxv << ")\n";
    }

    std::cout << "  best tour (indices): ";
    if (res.bestTour) {
        print_tour_brief(res.bestTour);
        double L = tour_length(*res.bestTour, D);
        double P = tour_profit(*res.bestTour, data);
        std::cout << "    length: " << L << " profit: " << P << " obj: " << (P - L) << "\n";
        size_t n = data->size();
        std::vector<char> seen(n, 0);
        for (int v : *res.bestTour) if (v >= 0 && static_cast<size_t>(v) < n) seen[v] = 1;
        std::cout << "    tour size: " << res.bestTour->size() << " unique visited: " << std::accumulate(seen.begin(), seen.end(), 0) << "\n";
    }
    else {
        std::cout << "(none)\n";
    }
    std::cout << "----------------------------------------\n";
}

// -----------------------------
// Zapis surowych wyników do CSV
// -----------------------------
static void write_results_csv_row(std::ofstream& ofs, const std::string& method, const std::string& instanceName,
    int run, int start, double lengthPhaseI, double objAfter, int visitedAfter, unsigned int seed, const std::vector<int>* tour) {
    ofs << method << "," << instanceName << "," << run << "," << start << "," << seed << "," << lengthPhaseI << "," << objAfter << "," << visitedAfter << ",";
    if (tour) {
        for (size_t i = 0; i < tour->size(); ++i) {
            if (i) ofs << " ";
            ofs << (*tour)[i];
        }
    }
    ofs << "\n";
}

// -----------------------------
// MAIN
// -----------------------------
int main(int argc, char** argv) {
    try {
        std::string path = (argc > 1) ? argv[1] : "TSPA.csv";
        char sep = ';';
        auto data = csv_to_2d_double_ptr(path, sep);
        if (!data || data->empty()) {
            std::cerr << "Brak danych w pliku CSV lub blad wczytywania\n";
            return 1;
        }
        size_t n = data->size();
        std::cout << "Wczytano " << n << " wierszy.\n";

        // macierz odległości
        auto D = compute_distance_matrix(data);

        // profit vector
        std::vector<double> profit(n, 0.0);
        for (size_t i = 0; i < n; ++i) profit[i] = ((*data)[i].size() > 2) ? (*data)[i][2] : 0.0;

        // RNG i seed
        int fixedSeed = -1; // ustaw na >=0 aby mieć powtarzalność, -1 = losowo
        std::random_device rd;
        unsigned int seed_base = (fixedSeed >= 0) ? static_cast<unsigned int>(fixedSeed) : rd();
        std::mt19937 rng(seed_base);

        int runs = 200;

        // przygotuj plik CSV wyników
        std::ofstream ofs("results.csv");
        ofs << "method,instance,run,start,seed,lengthPhaseI,objAfter,visitedAfter,tour\n";

        // wrappery heurystyk (start -> new tour)
        auto nn_wrapper = [&](int start)->std::vector<int>*{
            return nearest_neighbor(D, data, false, 0.0, start);
        };
        double profitWeightNN = 1.0;
        auto nn_profit_wrapper = [&](int start)->std::vector<int>*{
            return nearest_neighbor(D, data, true, profitWeightNN, start);
        };
        auto gc_wrapper = [&](int start)->std::vector<int>*{
            return greedy_cycle_build(D, data, false, 0.0, start);
        };
        double profitWeightGC = 1.0;
        auto gc_profit_wrapper = [&](int start)->std::vector<int>*{
            return greedy_cycle_build(D, data, true, profitWeightGC, start);
        };
        auto regret_wrapper = [&](int start)->std::vector<int>*{
            return regret_2(D, data, start);
        };
        auto wregret_wrapper = [&](int start)->std::vector<int>*{
            return weighted_regret_2(D, data, 1.0, 1.0, start);
        };
        // seed10 wrapper uses rng; to ensure deterministic starts we will create per-run rng with seed_base+run
        auto seed10_wrapper_factory = [&](double seedFraction)->HeuristicFn {
            return [=](int start)->std::vector<int>*{
                // create rng seeded by seed_base + start to vary per run deterministycznie
                unsigned int s = seed_base + static_cast<unsigned int>(start);
                std::mt19937 rng_local(s);
                return seed10_percent_expand(D, data, seedFraction, rng_local);
            };
        };

        // Lista heurystyk do testów
        struct HeurDesc { std::string name; HeuristicFn fn; bool doPrune; };
        std::vector<HeurDesc> heuristics = {
            {"NearestNeighbor (NNa)", nn_wrapper, true},
            {"NearestNeighbor (NN with profit)", nn_profit_wrapper, true},
            {"GreedyCycle (GCa)", gc_wrapper, true},
            {"GreedyCycle (GC with profit)", gc_profit_wrapper, true},
            {"Regret-2", regret_wrapper, true},
            {"WeightedRegret-2", wregret_wrapper, true},
            {"Seed10%+GreedyExpand", seed10_wrapper_factory(0.10), true} // [W10]
        };

        // Uruchom eksperymenty i zapis wyników
        for (auto& h : heuristics) {
            ExperimentResult res;
            // uruchamiamy runs razy; start = run % n (deterministyczne pokrycie)
            for (int run = 0; run < runs; ++run) {
                int start = run % static_cast<int>(n);
                unsigned int run_seed = seed_base + static_cast<unsigned int>(run);
                // jeśli heurystyka korzysta z globalnego RNG (np. random_solution), utwórz lokalny rng
                std::mt19937 rng_run(run_seed);
                // wywołanie heurystyki
                std::vector<int>* tour = h.fn(start);
                if (!tour) {
                    res.allObjs.push_back(-std::numeric_limits<double>::infinity());
                    res.lengthPhaseI.push_back(-std::numeric_limits<double>::infinity());
                    res.visitedAfter.push_back(0);
                    write_results_csv_row(ofs, h.name, path, run, start, -1.0, -std::numeric_limits<double>::infinity(), 0, run_seed, nullptr);
                    continue;
                }
                // walidacja: po fazie I (przed prune) jeśli heurystyka powinna tworzyć Hamiltona, sprawdź to
                // (dla NN, GC, regret, weighted_regret i seed10+expand powinniśmy mieć tour.size()==n)
                if (h.name.find("NearestNeighbor") != std::string::npos ||
                    h.name.find("GreedyCycle") != std::string::npos ||
                    h.name.find("Regret") != std::string::npos ||
                    h.name.find("Seed10%") != std::string::npos) {
                    // assert Hamiltona przed prune
                    // (jeśli seed10 variant B używany, to może być mniejszy; tutaj implementacja rozbudowuje do n)
                    assert(tour->size() == static_cast<int>(n));
                }
                double length_before_prune = tour_length(*tour, D);
                // zapis lengthPhaseI
                res.lengthPhaseI.push_back(length_before_prune);

                // prune
                if (h.doPrune) {
                    prune_remove_improving(*tour, D, profit);
                }
                double obj_after = objective_of_tour(*tour, D, data);
                res.allObjs.push_back(obj_after);
                res.visitedAfter.push_back(static_cast<int>(tour->size()));
                // zapis do CSV (surowe dane)
                write_results_csv_row(ofs, h.name, path, run, start, length_before_prune, obj_after, static_cast<int>(tour->size()), run_seed, tour);

                if (obj_after < res.minv) res.minv = obj_after;
                if (obj_after > res.maxv) res.maxv = obj_after;
                if (obj_after > res.bestObj) {
                    if (res.bestTour) delete res.bestTour;
                    res.bestTour = tour;
                    res.bestObj = obj_after;
                }
                else {
                    delete tour;
                }
            }
            // oblicz statystyki dla res.allObjs
            double sum = 0.0; int cnt = 0;
            for (double v : res.allObjs) if (std::isfinite(v)) { sum += v; ++cnt; }
            res.mean = (cnt > 0) ? sum / cnt : 0.0;
            double var = 0.0;
            for (double v : res.allObjs) if (std::isfinite(v)) var += (v - res.mean) * (v - res.mean);
            res.stddev = (cnt > 1) ? std::sqrt(var / (cnt - 1)) : 0.0;
            if (res.minv == std::numeric_limits<double>::infinity()) res.minv = 0.0;
            if (res.maxv == -std::numeric_limits<double>::infinity()) res.maxv = 0.0;

            // wypisz statystyki i najlepszy tour
            print_stats_and_best(h.name, res, data, D);
            if (res.bestTour) delete res.bestTour;
        }

        ofs.close();
        delete D;
        delete data;

        std::cout << "All experiments finished. Results saved to results.csv\n";
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
        return 2;
    }
}
