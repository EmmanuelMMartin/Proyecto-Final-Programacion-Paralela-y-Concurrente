#!/usr/bin/env bash
# ==============================================================================
# Script de Benchmarking — Fractal Mandelbrot + Convolución Gaussiana
# ==============================================================================
# Este script ejecuta benchmarks variando hilos, planificadores, tamaños de
# chunk y modos de histograma. Los resultados se guardan en archivos CSV.
# ==============================================================================

set -euo pipefail

# --- Configuración general ---
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BINARY="${PROJECT_DIR}/bin/fractal_par"
RESULTS_DIR="${PROJECT_DIR}/results"
OUTPUT_DIR="${PROJECT_DIR}/output"

# Resolución reducida para benchmarks rápidos
WIDTH=1920
HEIGHT=1080
MAX_ITER=500

# Número de repeticiones por prueba (se promedia el resultado)
NUM_RUNS=3

# Archivo temporal para la imagen de salida
TEMP_OUTPUT="${OUTPUT_DIR}/benchmark_temp.ppm"

# --- Colores para la salida ---
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # Sin color

# ==============================================================================
# Funciones auxiliares
# ==============================================================================

# Imprimir encabezado de sección
print_header() {
    echo -e "\n${BLUE}================================================================${NC}"
    echo -e "${BLUE}  $1${NC}"
    echo -e "${BLUE}================================================================${NC}\n"
}

# Imprimir progreso
print_progress() {
    echo -e "  ${YELLOW}[>>]${NC} $1"
}

# Imprimir resultado exitoso
print_success() {
    echo -e "  ${GREEN}[OK]${NC} $1"
}

# Imprimir error
print_error() {
    echo -e "  ${RED}[ERROR]${NC} $1"
}

# Ejecutar una prueba N veces y calcular el promedio
# Argumentos: descripción, argumentos del programa
# Retorna: tiempos separados por coma (mandelbrot, convolution, histogram, total)
run_averaged() {
    local description="$1"
    shift
    local args=("$@")

    local sum_mandelbrot=0
    local sum_convolution=0
    local sum_histogram=0
    local sum_total=0

    for ((run = 1; run <= NUM_RUNS; run++)); do
        # Ejecutar el programa y capturar la salida
        local output
        output=$("${BINARY}" "${args[@]}" --output "${TEMP_OUTPUT}" 2>&1) || {
            print_error "Fallo en ejecución: ${description} (intento ${run})"
            return 1
        }

        # Extraer tiempos de la salida del programa
        # Se espera formato: "TIME_MANDELBROT=X.XXX TIME_CONVOLUTION=X.XXX TIME_HISTOGRAM=X.XXX TIME_TOTAL=X.XXX"
        local t_mandelbrot t_convolution t_histogram t_total
        t_mandelbrot=$(echo "${output}" | grep -oP 'TIME_MANDELBROT=\K[0-9.]+' || echo "0")
        t_convolution=$(echo "${output}" | grep -oP 'TIME_CONVOLUTION=\K[0-9.]+' || echo "0")
        t_histogram=$(echo "${output}" | grep -oP 'TIME_HISTOGRAM=\K[0-9.]+' || echo "0")
        t_total=$(echo "${output}" | grep -oP 'TIME_TOTAL=\K[0-9.]+' || echo "0")

        sum_mandelbrot=$(echo "${sum_mandelbrot} + ${t_mandelbrot}" | bc)
        sum_convolution=$(echo "${sum_convolution} + ${t_convolution}" | bc)
        sum_histogram=$(echo "${sum_histogram} + ${t_histogram}" | bc)
        sum_total=$(echo "${sum_total} + ${t_total}" | bc)
    done

    # Calcular promedios
    local avg_mandelbrot avg_convolution avg_histogram avg_total
    avg_mandelbrot=$(echo "scale=6; ${sum_mandelbrot} / ${NUM_RUNS}" | bc)
    avg_convolution=$(echo "scale=6; ${sum_convolution} / ${NUM_RUNS}" | bc)
    avg_histogram=$(echo "scale=6; ${sum_histogram} / ${NUM_RUNS}" | bc)
    avg_total=$(echo "scale=6; ${sum_total} / ${NUM_RUNS}" | bc)

    echo "${avg_mandelbrot},${avg_convolution},${avg_histogram},${avg_total}"
}

# ==============================================================================
# Preparación
# ==============================================================================

print_header "PREPARACIÓN DEL ENTORNO"

# Crear directorios necesarios
mkdir -p "${RESULTS_DIR}" "${OUTPUT_DIR}"
print_success "Directorios creados"

# Compilar el proyecto
print_progress "Compilando el proyecto..."
cd "${PROJECT_DIR}"
make clean > /dev/null 2>&1 || true
make parallel
print_success "Compilación completada"

# Verificar que el binario existe
if [[ ! -f "${BINARY}" ]]; then
    # Intentar con extensión .exe (Windows/MSYS2)
    if [[ -f "${BINARY}.exe" ]]; then
        BINARY="${BINARY}.exe"
    else
        print_error "No se encontró el binario: ${BINARY}"
        exit 1
    fi
fi

print_success "Binario encontrado: ${BINARY}"
echo ""

# ==============================================================================
# Benchmark 1: Variación del número de hilos
# ==============================================================================

print_header "BENCHMARK 1: VARIACIÓN DEL NÚMERO DE HILOS"

THREADS_CSV="${RESULTS_DIR}/benchmark_threads.csv"
echo "threads,time_mandelbrot,time_convolution,time_histogram,time_total" > "${THREADS_CSV}"

THREAD_COUNTS=(1 2 4 6 8 10 12 14 16 20 24 32)

for threads in "${THREAD_COUNTS[@]}"; do
    print_progress "Probando con ${threads} hilo(s)... (${NUM_RUNS} repeticiones)"

    result=$(run_averaged "threads=${threads}" \
        --width ${WIDTH} --height ${HEIGHT} --max-iter ${MAX_ITER} \
        --threads "${threads}" --scheduler static)

    if [[ $? -eq 0 && -n "${result}" ]]; then
        echo "${threads},${result}" >> "${THREADS_CSV}"
        print_success "Hilos=${threads}: ${result}"
    else
        print_error "Fallo con ${threads} hilos"
    fi
done

print_success "Resultados guardados en: ${THREADS_CSV}"

# ==============================================================================
# Benchmark 2: Comparación de planificadores y tamaños de chunk
# ==============================================================================

print_header "BENCHMARK 2: PLANIFICADORES Y TAMAÑOS DE CHUNK"

SCHEDULERS_CSV="${RESULTS_DIR}/benchmark_schedulers.csv"
echo "scheduler,chunk_size,threads,time_mandelbrot,time_convolution,time_total" > "${SCHEDULERS_CSV}"

SCHEDULERS=(static dynamic guided)
CHUNK_SIZES=(1 10 50 100 500)
BENCH_THREADS=8  # Número fijo de hilos para esta prueba

for scheduler in "${SCHEDULERS[@]}"; do
    for chunk in "${CHUNK_SIZES[@]}"; do
        print_progress "Planificador=${scheduler}, chunk=${chunk}, hilos=${BENCH_THREADS}"

        result=$(run_averaged "scheduler=${scheduler},chunk=${chunk}" \
            --width ${WIDTH} --height ${HEIGHT} --max-iter ${MAX_ITER} \
            --threads "${BENCH_THREADS}" --scheduler "${scheduler}" --chunk "${chunk}")

        if [[ $? -eq 0 && -n "${result}" ]]; then
            # Extraer solo mandelbrot, convolution, total (sin histogram)
            t_mandelbrot=$(echo "${result}" | cut -d',' -f1)
            t_convolution=$(echo "${result}" | cut -d',' -f2)
            t_total=$(echo "${result}" | cut -d',' -f4)
            echo "${scheduler},${chunk},${BENCH_THREADS},${t_mandelbrot},${t_convolution},${t_total}" >> "${SCHEDULERS_CSV}"
            print_success "${scheduler}/chunk=${chunk}: total=${t_total}s"
        else
            print_error "Fallo con ${scheduler}, chunk=${chunk}"
        fi
    done
done

print_success "Resultados guardados en: ${SCHEDULERS_CSV}"

# ==============================================================================
# Benchmark 3: Modos de histograma
# ==============================================================================

print_header "BENCHMARK 3: MODOS DE HISTOGRAMA"

HISTOGRAM_CSV="${RESULTS_DIR}/benchmark_histogram.csv"
echo "mode,threads,time_histogram,time_total" > "${HISTOGRAM_CSV}"

HISTOGRAM_MODES=(sequential atomic critical reduction false_sharing)
HISTOGRAM_THREADS=(1 2 4 8 16)

for mode in "${HISTOGRAM_MODES[@]}"; do
    for threads in "${HISTOGRAM_THREADS[@]}"; do
        # El modo secuencial solo tiene sentido con 1 hilo
        if [[ "${mode}" == "sequential" && "${threads}" -ne 1 ]]; then
            continue
        fi

        print_progress "Histograma=${mode}, hilos=${threads}"

        result=$(run_averaged "histogram=${mode},threads=${threads}" \
            --width ${WIDTH} --height ${HEIGHT} --max-iter ${MAX_ITER} \
            --threads "${threads}" --histogram "${mode}")

        if [[ $? -eq 0 && -n "${result}" ]]; then
            t_histogram=$(echo "${result}" | cut -d',' -f3)
            t_total=$(echo "${result}" | cut -d',' -f4)
            echo "${mode},${threads},${t_histogram},${t_total}" >> "${HISTOGRAM_CSV}"
            print_success "${mode}/hilos=${threads}: histogram=${t_histogram}s"
        else
            print_error "Fallo con histograma=${mode}, hilos=${threads}"
        fi
    done
done

print_success "Resultados guardados en: ${HISTOGRAM_CSV}"

# ==============================================================================
# Limpieza y resumen
# ==============================================================================

print_header "RESUMEN DE BENCHMARKS"

# Eliminar archivo temporal
rm -f "${TEMP_OUTPUT}"

echo -e "Archivos generados:"
echo -e "  ${GREEN}✓${NC} ${THREADS_CSV}"
echo -e "  ${GREEN}✓${NC} ${SCHEDULERS_CSV}"
echo -e "  ${GREEN}✓${NC} ${HISTOGRAM_CSV}"
echo ""
echo -e "${GREEN}¡Benchmarks completados exitosamente!${NC}"
echo -e "Ejecute ${YELLOW}python scripts/plot_results.py${NC} para generar las gráficas."
echo ""
