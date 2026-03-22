#pragma once

// Konfiguracja funkcji celu.
//
// Wzór: node_value_sign * Σ(trzecia_kolumna) − Σ(odległość)
//
// Odległość jest zawsze kosztem (odejmujemy). Jedyna konfiguracja
// to czy trzecia kolumna jest zyskiem czy kosztem.
struct ObjectiveConfig {
    int node_value_sign = +1; // +1: trzecia kolumna to ZYSK  → dodajemy
                              // -1: trzecia kolumna to KOSZT → odejmujemy

    static ObjectiveConfig profit() { return {+1}; } // zysk  − odległość
    static ObjectiveConfig cost()   { return {-1}; } // −koszt − odległość
};
