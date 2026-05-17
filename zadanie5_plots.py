"""
Zadanie 5 — testy globalnej wypuklosci.

Wczytuje 1000 lokalnych optimow (results/{TSPA,TSPB}_local_optima.csv) oraz
najlepsze rozwiazanie zad. 4 (results/{TSPA,TSPB}_ILS_path.csv). Liczy
podobienstwo (wspolne wierzcholki, wspolne krawedzie) kazdego lok. optimum
do (a) najlepszego rozwiazania, (b) srednia do pozostalych 999 optimow.
Rysuje wykresy obj -> similarity i liczy wspolczynnik korelacji Pearsona.
"""
import os
import csv
import math
import numpy as np
import matplotlib.pyplot as plt

RESULTS_DIR = "results"
PLOTS_DIR = os.path.join(RESULTS_DIR, "plots")
os.makedirs(PLOTS_DIR, exist_ok=True)


def load_local_optima(path):
    objs, cycles = [], []
    with open(path) as f:
        reader = csv.DictReader(f)
        for row in reader:
            objs.append(int(row["objective"]))
            cycles.append([int(x) for x in row["cycle"].split()])
    return objs, cycles


def load_best_cycle(path):
    rows = []
    with open(path) as f:
        reader = csv.DictReader(f)
        for row in reader:
            rows.append((int(row["step"]), int(row["node_id"])))
    rows.sort()
    return [nid for _, nid in rows]


def edges_of(cycle):
    n = len(cycle)
    edges = set()
    for i in range(n):
        a, b = cycle[i], cycle[(i + 1) % n]
        edges.add((a, b) if a < b else (b, a))
    return edges


def common_vertices(a_set, b_set):
    return len(a_set & b_set)


def analyze(inst_name):
    optima_path = os.path.join(RESULTS_DIR, f"{inst_name}_local_optima.csv")
    best_path = os.path.join(RESULTS_DIR, f"{inst_name}_ILS_path.csv")
    objs, cycles = load_local_optima(optima_path)
    best_cycle = load_best_cycle(best_path)

    vsets = [set(c) for c in cycles]
    esets = [edges_of(c) for c in cycles]
    best_vset = set(best_cycle)
    best_eset = edges_of(best_cycle)

    n = len(cycles)

    sim_v_best = np.array([common_vertices(v, best_vset) for v in vsets], dtype=float)
    sim_e_best = np.array([len(e & best_eset) for e in esets], dtype=float)

    # Srednie podobienstwo do pozostalych n-1 optimow.
    sim_v_avg = np.zeros(n)
    sim_e_avg = np.zeros(n)
    for i in range(n):
        sv = 0
        se = 0
        for j in range(n):
            if i == j:
                continue
            sv += len(vsets[i] & vsets[j])
            se += len(esets[i] & esets[j])
        sim_v_avg[i] = sv / (n - 1)
        sim_e_avg[i] = se / (n - 1)

    objs_arr = np.array(objs, dtype=float)

    def corr(a, b):
        if np.std(a) == 0 or np.std(b) == 0:
            return float("nan")
        return float(np.corrcoef(a, b)[0, 1])

    series = {
        "vertices_vs_best": sim_v_best,
        "edges_vs_best": sim_e_best,
        "vertices_avg_others": sim_v_avg,
        "edges_avg_others": sim_e_avg,
    }
    corrs = {k: corr(objs_arr, v) for k, v in series.items()}

    # 4 wykresy na 1 figure.
    fig, axes = plt.subplots(2, 2, figsize=(12, 9))
    plot_specs = [
        ("vertices_vs_best",
         "Wspolne wierzcholki z najlepszym rozwiazaniem",
         "liczba wspolnych wierzcholkow", axes[0, 0]),
        ("edges_vs_best",
         "Wspolne krawedzie z najlepszym rozwiazaniem",
         "liczba wspolnych krawedzi", axes[0, 1]),
        ("vertices_avg_others",
         "Srednia liczba wspolnych wierzcholkow (do pozostalych)",
         "srednia liczba wspolnych wierzcholkow", axes[1, 0]),
        ("edges_avg_others",
         "Srednia liczba wspolnych krawedzi (do pozostalych)",
         "srednia liczba wspolnych krawedzi", axes[1, 1]),
    ]
    for key, title, ylabel, ax in plot_specs:
        ax.scatter(objs_arr, series[key], s=8, alpha=0.5)
        ax.set_xlabel("wartosc funkcji celu")
        ax.set_ylabel(ylabel)
        ax.set_title(f"{title}\nkorelacja Pearsona = {corrs[key]:.3f}")
        ax.grid(True, alpha=0.3)

    fig.suptitle(f"{inst_name} — globalna wypuklosc (1000 lok. optimow, greedy LS)",
                 fontsize=13)
    fig.tight_layout(rect=[0, 0, 1, 0.97])
    out_path = os.path.join(PLOTS_DIR, f"{inst_name}_zadanie5.png")
    fig.savefig(out_path, dpi=130)
    plt.close(fig)
    print(f"Zapisano wykres: {out_path}")

    return corrs, objs_arr.min(), objs_arr.max(), objs_arr.mean()


def main():
    summary_rows = []
    for inst in ("TSPA", "TSPB"):
        corrs, mn, mx, avg = analyze(inst)
        print(f"\n=== {inst} ===")
        print(f"  obj: min={mn:.0f}  max={mx:.0f}  avg={avg:.1f}")
        for k, v in corrs.items():
            print(f"  korelacja({k}) = {v:.4f}")
        summary_rows.append({
            "instance": inst,
            "obj_min": mn, "obj_max": mx, "obj_avg": avg,
            **{f"corr_{k}": v for k, v in corrs.items()},
        })

    summary_path = os.path.join(RESULTS_DIR, "zadanie5_correlations.csv")
    if summary_rows:
        keys = list(summary_rows[0].keys())
        with open(summary_path, "w", newline="") as f:
            w = csv.DictWriter(f, fieldnames=keys)
            w.writeheader()
            w.writerows(summary_rows)
    print(f"\nZapisano podsumowanie: {summary_path}")


if __name__ == "__main__":
    main()
