#!/usr/bin/env python3
# ==============================================================================
# Script de Generación de Gráficas — Resultados de Benchmarks
# ==============================================================================
# Este script lee los archivos CSV generados por los benchmarks y produce
# gráficas de calidad para publicación (300 DPI) con etiquetas en español.
# ==============================================================================

import os
import sys
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
from pathlib import Path

# --- Configuración de rutas ---
SCRIPT_DIR = Path(__file__).parent.resolve()
PROJECT_DIR = SCRIPT_DIR.parent
RESULTS_DIR = PROJECT_DIR / "results"

# --- Configuración visual global ---
# Paleta de colores profesional
COLORS = {
    "primary": "#2E86AB",       # Azul principal
    "secondary": "#A23B72",     # Magenta
    "tertiary": "#F18F01",      # Naranja
    "quaternary": "#C73E1D",    # Rojo
    "quinary": "#3B1F2B",       # Púrpura oscuro
    "success": "#2ECC71",       # Verde
    "neutral": "#95A5A6",       # Gris
}

COLOR_PALETTE = ["#2E86AB", "#A23B72", "#F18F01", "#C73E1D", "#2ECC71",
                 "#9B59B6", "#1ABC9C", "#E74C3C", "#3498DB", "#F39C12"]

# Estilo global de matplotlib
plt.rcParams.update({
    "figure.figsize": (10, 6),
    "figure.dpi": 100,
    "savefig.dpi": 300,
    "savefig.bbox_inches": "tight",
    "font.size": 12,
    "axes.titlesize": 14,
    "axes.labelsize": 12,
    "legend.fontsize": 10,
    "xtick.labelsize": 10,
    "ytick.labelsize": 10,
    "axes.grid": True,
    "grid.alpha": 0.3,
    "grid.linestyle": "--",
    "axes.spines.top": False,
    "axes.spines.right": False,
})


def load_csv(filename):
    """Cargar un archivo CSV de resultados."""
    filepath = RESULTS_DIR / filename
    if not filepath.exists():
        print(f"  ⚠ Archivo no encontrado: {filepath}")
        return None
    try:
        df = pd.read_csv(filepath)
        print(f"  ✓ Cargado: {filename} ({len(df)} filas)")
        return df
    except Exception as e:
        print(f"  ✗ Error al cargar {filename}: {e}")
        return None


# ==============================================================================
# Gráfica 1: Tiempo de Ejecución vs Número de Hilos
# ==============================================================================
def plot_execution_time_vs_threads(df):
    """Generar gráfica de tiempo de ejecución vs número de hilos."""
    fig, ax = plt.subplots(figsize=(11, 7))

    # Graficar cada componente de tiempo
    ax.plot(df["threads"], df["time_mandelbrot"], marker="o", linewidth=2,
            markersize=8, color=COLORS["primary"], label="Mandelbrot",
            zorder=5)
    ax.plot(df["threads"], df["time_convolution"], marker="s", linewidth=2,
            markersize=8, color=COLORS["secondary"], label="Convolución",
            zorder=5)

    # Si existe columna de histograma
    if "time_histogram" in df.columns:
        ax.plot(df["threads"], df["time_histogram"], marker="^", linewidth=2,
                markersize=8, color=COLORS["tertiary"], label="Histograma",
                zorder=5)

    ax.plot(df["threads"], df["time_total"], marker="D", linewidth=2.5,
            markersize=9, color=COLORS["quaternary"], label="Total",
            zorder=5, linestyle="--")

    # Configuración de ejes
    ax.set_xlabel("Número de Hilos")
    ax.set_ylabel("Tiempo de Ejecución (s)")
    ax.set_title("Tiempo de Ejecución vs Número de Hilos")
    ax.set_xticks(df["threads"])
    ax.legend(loc="upper right", framealpha=0.9)
    ax.set_xlim(left=0)
    ax.set_ylim(bottom=0)

    # Guardar la gráfica
    output_path = RESULTS_DIR / "graph_execution_time.png"
    fig.savefig(output_path)
    plt.close(fig)
    print(f"  ✓ Gráfica guardada: {output_path.name}")


# ==============================================================================
# Gráfica 2: Speedup vs Número de Hilos
# ==============================================================================
def plot_speedup_vs_threads(df):
    """Generar gráfica de speedup con referencia lineal y curva de Amdahl."""
    fig, ax = plt.subplots(figsize=(11, 7))

    # Calcular speedup
    t1 = df.loc[df["threads"] == 1, "time_total"].values
    if len(t1) == 0:
        print("  ⚠ No hay datos para 1 hilo, no se puede calcular speedup")
        return
    t1 = t1[0]

    threads = df["threads"].values
    speedup = t1 / df["time_total"].values

    # Estimar fracción paralela (p) para la curva de Amdahl
    # Usar el speedup con el mayor número de hilos para la estimación
    max_threads = threads[-1]
    max_speedup = speedup[-1]
    # S = 1 / ((1-p) + p/N)  =>  p = (1 - 1/S) / (1 - 1/N)
    if max_speedup > 1 and max_threads > 1:
        p_estimated = (1 - 1 / max_speedup) / (1 - 1 / max_threads)
        p_estimated = min(p_estimated, 0.999)  # Limitar para evitar valores extremos
    else:
        p_estimated = 0.9  # Valor por defecto

    # Generar curva de Amdahl
    amdahl_threads = np.linspace(1, max(threads) * 1.1, 100)
    amdahl_speedup = 1.0 / ((1 - p_estimated) + p_estimated / amdahl_threads)

    # Graficar speedup ideal (lineal)
    ax.plot(threads, threads, linewidth=1.5, color=COLORS["neutral"],
            linestyle=":", label="Speedup Ideal (lineal)", alpha=0.7)

    # Graficar curva de Amdahl
    ax.plot(amdahl_threads, amdahl_speedup, linewidth=2, color=COLORS["tertiary"],
            linestyle="--", label=f"Ley de Amdahl (p={p_estimated:.3f})", alpha=0.8)

    # Graficar speedup real
    ax.plot(threads, speedup, marker="o", linewidth=2.5, markersize=9,
            color=COLORS["primary"], label="Speedup Real", zorder=5)

    # Anotar valores de speedup
    for i, (t, s) in enumerate(zip(threads, speedup)):
        if i % 2 == 0 or t == threads[-1]:  # Anotar cada 2 puntos y el último
            ax.annotate(f"{s:.2f}x", (t, s), textcoords="offset points",
                        xytext=(0, 12), ha="center", fontsize=9,
                        color=COLORS["primary"], fontweight="bold")

    # Configuración de ejes
    ax.set_xlabel("Número de Hilos")
    ax.set_ylabel("Speedup (T₁ / Tₙ)")
    ax.set_title("Speedup vs Número de Hilos")
    ax.set_xticks(threads)
    ax.legend(loc="upper left", framealpha=0.9)
    ax.set_xlim(left=0)
    ax.set_ylim(bottom=0)

    # Guardar la gráfica
    output_path = RESULTS_DIR / "graph_speedup.png"
    fig.savefig(output_path)
    plt.close(fig)
    print(f"  ✓ Gráfica guardada: {output_path.name}")


# ==============================================================================
# Gráfica 3: Comparación de Planificadores
# ==============================================================================
def plot_scheduler_comparison(df):
    """Generar gráfica de barras agrupadas comparando planificadores."""
    fig, ax = plt.subplots(figsize=(13, 7))

    schedulers = df["scheduler"].unique()
    chunk_sizes = sorted(df["chunk_size"].unique())

    x = np.arange(len(chunk_sizes))
    bar_width = 0.25
    offsets = np.arange(len(schedulers)) - (len(schedulers) - 1) / 2

    for i, scheduler in enumerate(schedulers):
        subset = df[df["scheduler"] == scheduler].sort_values("chunk_size")
        times = []
        for chunk in chunk_sizes:
            row = subset[subset["chunk_size"] == chunk]
            if not row.empty:
                times.append(row["time_total"].values[0])
            else:
                times.append(0)

        bars = ax.bar(x + offsets[i] * bar_width, times, bar_width,
                      label=scheduler.capitalize(), color=COLOR_PALETTE[i],
                      edgecolor="white", linewidth=0.5, zorder=3)

        # Anotar valores sobre cada barra
        for bar, val in zip(bars, times):
            if val > 0:
                ax.text(bar.get_x() + bar.get_width() / 2, bar.get_height(),
                        f"{val:.3f}", ha="center", va="bottom", fontsize=8,
                        fontweight="bold")

    # Configuración de ejes
    ax.set_xlabel("Tamaño de Chunk")
    ax.set_ylabel("Tiempo Total (s)")
    ax.set_title("Comparación de Planificadores OpenMP por Tamaño de Chunk")
    ax.set_xticks(x)
    ax.set_xticklabels([str(c) for c in chunk_sizes])
    ax.legend(title="Planificador", framealpha=0.9)
    ax.set_ylim(bottom=0)

    # Guardar la gráfica
    output_path = RESULTS_DIR / "graph_schedulers.png"
    fig.savefig(output_path)
    plt.close(fig)
    print(f"  ✓ Gráfica guardada: {output_path.name}")


# ==============================================================================
# Gráfica 4: Comparación de Modos de Histograma
# ==============================================================================
def plot_histogram_comparison(df):
    """Generar gráfica de barras comparando modos de histograma."""
    fig, ax = plt.subplots(figsize=(11, 7))

    # Filtrar para un número fijo de hilos (usar el máximo disponible, excluyendo secuencial)
    parallel_df = df[df["mode"] != "sequential"]
    if parallel_df.empty:
        print("  ⚠ No hay datos paralelos para el histograma")
        return

    # Usar el mayor número de hilos disponible
    target_threads = parallel_df["threads"].max()

    # Obtener datos para cada modo
    modes = []
    times = []
    colors = []
    color_map = {
        "sequential": COLORS["neutral"],
        "atomic": COLORS["primary"],
        "critical": COLORS["secondary"],
        "reduction": COLORS["success"],
        "false_sharing": COLORS["quaternary"],
    }

    # Agregar secuencial primero
    seq_data = df[df["mode"] == "sequential"]
    if not seq_data.empty:
        modes.append("Secuencial")
        times.append(seq_data["time_histogram"].values[0])
        colors.append(color_map["sequential"])

    # Agregar modos paralelos
    mode_labels = {
        "atomic": "Atómico",
        "critical": "Sección Crítica",
        "reduction": "Reducción",
        "false_sharing": "False Sharing",
    }

    for mode_key, mode_label in mode_labels.items():
        row = parallel_df[(parallel_df["mode"] == mode_key) &
                          (parallel_df["threads"] == target_threads)]
        if not row.empty:
            modes.append(mode_label)
            times.append(row["time_histogram"].values[0])
            colors.append(color_map.get(mode_key, COLORS["neutral"]))

    # Crear gráfica de barras
    x = np.arange(len(modes))
    bars = ax.bar(x, times, color=colors, edgecolor="white", linewidth=0.5,
                  width=0.6, zorder=3)

    # Anotar valores sobre las barras
    for bar, val in zip(bars, times):
        ax.text(bar.get_x() + bar.get_width() / 2, bar.get_height(),
                f"{val:.4f}s", ha="center", va="bottom", fontsize=10,
                fontweight="bold")

    # Configuración de ejes
    ax.set_xlabel("Modo de Histograma")
    ax.set_ylabel("Tiempo de Histograma (s)")
    ax.set_title(f"Comparación de Modos de Histograma ({target_threads} hilos)")
    ax.set_xticks(x)
    ax.set_xticklabels(modes, rotation=15, ha="right")
    ax.set_ylim(bottom=0)

    # Guardar la gráfica
    output_path = RESULTS_DIR / "graph_histogram.png"
    fig.savefig(output_path)
    plt.close(fig)
    print(f"  ✓ Gráfica guardada: {output_path.name}")


# ==============================================================================
# Gráfica 5: Impacto de la Afinidad de Hilos
# ==============================================================================
def plot_affinity_impact(df):
    """Generar gráfica de barras del impacto de la afinidad de hilos."""
    fig, ax = plt.subplots(figsize=(13, 7))

    # Crear etiquetas combinadas
    labels = []
    times = []
    colors_list = []

    for idx, row in df.iterrows():
        bind = row["proc_bind"]
        places = row["places"]
        if bind == "none":
            label = "Sin afinidad\n(línea base)"
        else:
            label = f"{bind}\n{places}"
        labels.append(label)
        times.append(row["time_total"])

        # Color según proc_bind
        if bind == "none":
            colors_list.append(COLORS["neutral"])
        elif bind == "close":
            colors_list.append(COLORS["primary"])
        elif bind == "spread":
            colors_list.append(COLORS["secondary"])
        elif bind == "master":
            colors_list.append(COLORS["tertiary"])
        else:
            colors_list.append(COLORS["neutral"])

    x = np.arange(len(labels))
    bars = ax.bar(x, times, color=colors_list, edgecolor="white",
                  linewidth=0.5, width=0.7, zorder=3)

    # Línea de referencia (línea base)
    if len(times) > 0:
        baseline = times[0]
        ax.axhline(y=baseline, color=COLORS["neutral"], linestyle="--",
                   linewidth=1, alpha=0.7, label=f"Línea base ({baseline:.3f}s)")

    # Anotar valores y porcentajes
    for bar, val in zip(bars, times):
        # Valor absoluto
        ax.text(bar.get_x() + bar.get_width() / 2, bar.get_height(),
                f"{val:.3f}s", ha="center", va="bottom", fontsize=9,
                fontweight="bold")

    # Configuración de ejes
    ax.set_xlabel("Configuración de Afinidad (OMP_PROC_BIND / OMP_PLACES)")
    ax.set_ylabel("Tiempo Total (s)")
    ax.set_title("Impacto de la Afinidad de Hilos en el Rendimiento")
    ax.set_xticks(x)
    ax.set_xticklabels(labels, fontsize=9)
    ax.legend(loc="upper right", framealpha=0.9)
    ax.set_ylim(bottom=0)

    # Guardar la gráfica
    output_path = RESULTS_DIR / "graph_affinity.png"
    fig.savefig(output_path)
    plt.close(fig)
    print(f"  ✓ Gráfica guardada: {output_path.name}")


# ==============================================================================
# Función principal
# ==============================================================================
def main():
    print("=" * 64)
    print("  GENERACIÓN DE GRÁFICAS DE RESULTADOS")
    print("=" * 64)
    print()

    # Verificar que el directorio de resultados existe
    if not RESULTS_DIR.exists():
        print(f"✗ El directorio de resultados no existe: {RESULTS_DIR}")
        print("  Ejecute primero los benchmarks: bash scripts/run_benchmarks.sh")
        sys.exit(1)

    # Cargar datos
    print("Cargando archivos CSV...")
    df_threads = load_csv("benchmark_threads.csv")
    df_schedulers = load_csv("benchmark_schedulers.csv")
    df_histogram = load_csv("benchmark_histogram.csv")
    df_affinity = load_csv("benchmark_affinity.csv")
    print()

    # Generar gráficas
    graphs_generated = 0

    # Gráfica 1: Tiempo vs Hilos
    if df_threads is not None and not df_threads.empty:
        print("Generando: Tiempo de Ejecución vs Número de Hilos...")
        plot_execution_time_vs_threads(df_threads)
        graphs_generated += 1

        # Gráfica 2: Speedup vs Hilos
        print("Generando: Speedup vs Número de Hilos...")
        plot_speedup_vs_threads(df_threads)
        graphs_generated += 1

    # Gráfica 3: Comparación de planificadores
    if df_schedulers is not None and not df_schedulers.empty:
        print("Generando: Comparación de Planificadores...")
        plot_scheduler_comparison(df_schedulers)
        graphs_generated += 1

    # Gráfica 4: Modos de histograma
    if df_histogram is not None and not df_histogram.empty:
        print("Generando: Comparación de Modos de Histograma...")
        plot_histogram_comparison(df_histogram)
        graphs_generated += 1

    # Gráfica 5: Impacto de afinidad
    if df_affinity is not None and not df_affinity.empty:
        print("Generando: Impacto de la Afinidad de Hilos...")
        plot_affinity_impact(df_affinity)
        graphs_generated += 1

    # Resumen
    print()
    print("=" * 64)
    if graphs_generated > 0:
        print(f"  ✓ {graphs_generated} gráfica(s) generada(s) en: {RESULTS_DIR}")
    else:
        print("  ⚠ No se generaron gráficas. Verifique los archivos CSV.")
    print("=" * 64)
    print()


if __name__ == "__main__":
    main()
