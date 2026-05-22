#!/usr/bin/env bash
# ==============================================================================
# Script de Pruebas de Afinidad de Hilos — OpenMP Thread Affinity
# ==============================================================================
# Este script evalúa el impacto de diferentes configuraciones de afinidad
# de hilos en el rendimiento del programa paralelo.
# ==============================================================================

set -euo pipefail

# --- Configuración general ---
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BINARY="${PROJECT_DIR}/bin/fractal_par"
RESULTS_DIR="${PROJECT_DIR}/results"
OUTPUT_DIR="${PROJECT_DIR}/output"

# Resolución reducida para benchmarks
WIDTH=1920
HEIGHT=1080
MAX_ITER=500

# Número de repeticiones por prueba
NUM_RUNS=3

# Número de hilos para las pruebas de afinidad
AFFINITY_THREADS=8

# Archivo temporal para la imagen de salida
TEMP_OUTPUT="${OUTPUT_DIR}/affinity_temp.ppm"

# --- Colores para la salida ---
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# ==============================================================================
# Funciones auxiliares
# ==============================================================================

print_header() {
    echo -e "\n${BLUE}================================================================${NC}"
    echo -e "${BLUE}  $1${NC}"
    echo -e "${BLUE}================================================================${NC}\n"
}

print_progress() {
    echo -e "  ${YELLOW}[>>]${NC} $1"
}

print_success() {
    echo -e "  ${GREEN}[OK]${NC} $1"
}

print_error() {
    echo -e "  ${RED}[ERROR]${NC} $1"
}

# Ejecutar una prueba N veces y calcular el promedio del tiempo total
run_averaged_total() {
    local description="$1"
    shift
    local env_vars=("$@")

    local sum_total=0

    for ((run = 1; run <= NUM_RUNS; run++)); do
        local output
        output=$(env "${env_vars[@]}" "${BINARY}" \
            --width ${WIDTH} --height ${HEIGHT} --max-iter ${MAX_ITER} \
            --threads "${AFFINITY_THREADS}" --output "${TEMP_OUTPUT}" 2>&1) || {
            print_error "Fallo en ejecución: ${description} (intento ${run})"
            return 1
        }

        local t_total
        t_total=$(echo "${output}" | grep -oP 'TIME_TOTAL=\K[0-9.]+' || echo "0")
        sum_total=$(echo "${sum_total} + ${t_total}" | bc)
    done

    local avg_total
    avg_total=$(echo "scale=6; ${sum_total} / ${NUM_RUNS}" | bc)
    echo "${avg_total}"
}

# ==============================================================================
# Preparación
# ==============================================================================

print_header "PRUEBAS DE AFINIDAD DE HILOS"

mkdir -p "${RESULTS_DIR}" "${OUTPUT_DIR}"

# Verificar que el binario existe
if [[ ! -f "${BINARY}" ]]; then
    if [[ -f "${BINARY}.exe" ]]; then
        BINARY="${BINARY}.exe"
    else
        print_error "No se encontró el binario. Compile primero con: make parallel"
        exit 1
    fi
fi

print_success "Binario encontrado: ${BINARY}"
echo -e "  Hilos: ${AFFINITY_THREADS}"
echo -e "  Resolución: ${WIDTH}x${HEIGHT}"
echo -e "  Repeticiones por prueba: ${NUM_RUNS}"

# ==============================================================================
# Prueba de línea base (sin configuración de afinidad)
# ==============================================================================

print_header "LÍNEA BASE (SIN AFINIDAD CONFIGURADA)"

AFFINITY_CSV="${RESULTS_DIR}/benchmark_affinity.csv"
echo "proc_bind,places,threads,time_total" > "${AFFINITY_CSV}"

# Línea base: sin variables de afinidad
print_progress "Ejecutando línea base..."
baseline_time=$(env OMP_NUM_THREADS="${AFFINITY_THREADS}" "${BINARY}" \
    --width ${WIDTH} --height ${HEIGHT} --max-iter ${MAX_ITER} \
    --threads "${AFFINITY_THREADS}" --output "${TEMP_OUTPUT}" 2>&1 | \
    grep -oP 'TIME_TOTAL=\K[0-9.]+' || echo "0")

# Calcular promedio de la línea base
sum_baseline=0
for ((run = 1; run <= NUM_RUNS; run++)); do
    t=$(env OMP_NUM_THREADS="${AFFINITY_THREADS}" "${BINARY}" \
        --width ${WIDTH} --height ${HEIGHT} --max-iter ${MAX_ITER} \
        --threads "${AFFINITY_THREADS}" --output "${TEMP_OUTPUT}" 2>&1 | \
        grep -oP 'TIME_TOTAL=\K[0-9.]+' || echo "0")
    sum_baseline=$(echo "${sum_baseline} + ${t}" | bc)
done
avg_baseline=$(echo "scale=6; ${sum_baseline} / ${NUM_RUNS}" | bc)

echo "none,none,${AFFINITY_THREADS},${avg_baseline}" >> "${AFFINITY_CSV}"
print_success "Línea base: ${avg_baseline}s"

# ==============================================================================
# Pruebas de combinaciones de afinidad
# ==============================================================================

print_header "COMBINACIONES DE OMP_PROC_BIND Y OMP_PLACES"

# Valores de OMP_PROC_BIND a probar
PROC_BIND_VALUES=(close spread master)

# Valores de OMP_PLACES a probar
PLACES_VALUES=(cores threads sockets)

for proc_bind in "${PROC_BIND_VALUES[@]}"; do
    for places in "${PLACES_VALUES[@]}"; do
        print_progress "OMP_PROC_BIND=${proc_bind}, OMP_PLACES=${places}"

        # Ejecutar con las variables de entorno configuradas
        sum_total=0
        success=true

        for ((run = 1; run <= NUM_RUNS; run++)); do
            output=$(env \
                OMP_NUM_THREADS="${AFFINITY_THREADS}" \
                OMP_PROC_BIND="${proc_bind}" \
                OMP_PLACES="${places}" \
                "${BINARY}" \
                --width ${WIDTH} --height ${HEIGHT} --max-iter ${MAX_ITER} \
                --threads "${AFFINITY_THREADS}" --output "${TEMP_OUTPUT}" 2>&1) || {
                print_error "Fallo: PROC_BIND=${proc_bind}, PLACES=${places} (intento ${run})"
                success=false
                break
            }

            t_total=$(echo "${output}" | grep -oP 'TIME_TOTAL=\K[0-9.]+' || echo "0")
            sum_total=$(echo "${sum_total} + ${t_total}" | bc)
        done

        if ${success}; then
            avg_total=$(echo "scale=6; ${sum_total} / ${NUM_RUNS}" | bc)
            echo "${proc_bind},${places},${AFFINITY_THREADS},${avg_total}" >> "${AFFINITY_CSV}"

            # Calcular diferencia porcentual vs línea base
            if [[ "${avg_baseline}" != "0" ]]; then
                pct_diff=$(echo "scale=2; ((${avg_total} - ${avg_baseline}) / ${avg_baseline}) * 100" | bc)
                if (( $(echo "${pct_diff} < 0" | bc -l) )); then
                    print_success "BIND=${proc_bind}, PLACES=${places}: ${avg_total}s (${pct_diff}% vs base)"
                else
                    print_success "BIND=${proc_bind}, PLACES=${places}: ${avg_total}s (+${pct_diff}% vs base)"
                fi
            else
                print_success "BIND=${proc_bind}, PLACES=${places}: ${avg_total}s"
            fi
        fi
    done
done

# ==============================================================================
# Limpieza y resumen
# ==============================================================================

print_header "RESUMEN DE PRUEBAS DE AFINIDAD"

rm -f "${TEMP_OUTPUT}"

echo -e "Archivo generado:"
echo -e "  ${GREEN}✓${NC} ${AFFINITY_CSV}"
echo ""

# Mostrar tabla resumen
echo -e "${BLUE}Resultados:${NC}"
echo "----------------------------------------------------"
printf "%-12s %-10s %-8s %-12s\n" "PROC_BIND" "PLACES" "HILOS" "TIEMPO (s)"
echo "----------------------------------------------------"
tail -n +2 "${AFFINITY_CSV}" | while IFS=',' read -r bind places threads time; do
    printf "%-12s %-10s %-8s %-12s\n" "${bind}" "${places}" "${threads}" "${time}"
done
echo "----------------------------------------------------"
echo ""
echo -e "${GREEN}¡Pruebas de afinidad completadas!${NC}"
echo -e "Ejecute ${YELLOW}python scripts/plot_results.py${NC} para generar las gráficas."
echo ""
