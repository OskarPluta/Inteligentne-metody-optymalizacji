#!/usr/bin/env python3
"""Generuje statyczne wizualizacje najlepszych ścieżek (PNG) do sprawozdania."""

from pathlib import Path

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

RESULTS_DIR = Path("results")
ALGOS = [
    "Random", "NNa", "NN", "GCa", "GC", "2-regret",
    "LSs_v_rnd", "LSs_v_h", "LSs_e_rnd", "LSs_e_h",
    "LSg_v_rnd", "LSg_v_h", "LSg_e_rnd", "LSg_e_h",
    "RW",
]
INSTANCES = ["TSPA", "TSPB"]


def plot_path(inst_name: str, algo_name: str, df: pd.DataFrame, out_dir: Path):
    safe = algo_name.replace("-", "_").replace(" ", "_")
    nodes_file = RESULTS_DIR / f"{inst_name}_nodes.csv"
    path_file = RESULTS_DIR / f"{inst_name}_{safe}_path.csv"

    if not path_file.exists():
        print(f"  SKIP {inst_name}/{algo_name} — brak {path_file.name}")
        return

    all_nodes = pd.read_csv(nodes_file)
    path_nodes = pd.read_csv(path_file)
    n_path = len(path_nodes)

    path_ids = set(path_nodes["node_id"])
    bg_nodes = all_nodes[~all_nodes["id"].isin(path_ids)]

    sub = df[(df["instance"] == inst_name) & (df["algorithm"] == algo_name)]
    best_obj = int(sub["max_obj"].iloc[0]) if not sub.empty else 0

    px = path_nodes["x"].to_numpy()
    py = path_nodes["y"].to_numpy()

    fig, ax = plt.subplots(figsize=(7, 6))
    fig.patch.set_facecolor("white")
    ax.set_facecolor("#f8f8f8")

    # Nieodwiedzone wierzchołki
    ax.scatter(bg_nodes["x"], bg_nodes["y"], c="#cccccc", s=12, zorder=1)

    # Krawędzie cyklu
    for i in range(n_path):
        ax.plot(
            [px[i], px[(i + 1) % n_path]],
            [py[i], py[(i + 1) % n_path]],
            color="#2196F3", alpha=0.6, linewidth=1.0, zorder=2,
        )

    # Wierzchołki w cyklu
    ax.scatter(px, py, c="#1565C0", s=20, zorder=3)

    ax.set_title(f"{inst_name} — {algo_name}\nf. celu = {best_obj}  |  wierzchołków: {n_path}",
                 fontsize=11, pad=10)
    ax.set_xlabel("x")
    ax.set_ylabel("y")
    ax.set_aspect("equal")
    plt.tight_layout()

    out_path = out_dir / f"{inst_name}_{safe}.pdf"
    fig.savefig(out_path, bbox_inches="tight")
    plt.close(fig)
    print(f"  {out_path}")


def main():
    out_dir = RESULTS_DIR / "plots"
    out_dir.mkdir(exist_ok=True)

    df = pd.read_csv(RESULTS_DIR / "results.csv")

    for inst in INSTANCES:
        for algo in ALGOS:
            plot_path(inst, algo, df, out_dir)

    print(f"\nWszystkie wykresy w {out_dir}/")


if __name__ == "__main__":
    main()
