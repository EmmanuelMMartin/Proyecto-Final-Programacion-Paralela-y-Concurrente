# ==============================================================================
# Makefile — Proyecto Fractal Mandelbrot + Convolución Gaussiana con OpenMP
# ==============================================================================

# Compilador
CC = gcc

# Banderas de compilación
CFLAGS_SEQ = -O2 -Wall -Wextra -std=c11
CFLAGS_PAR = -O2 -Wall -Wextra -std=c11 -fopenmp
CFLAGS_VEC = -O2 -Wall -Wextra -std=c11 -fopenmp -march=native -fopt-info-vec-optimized

# Banderas de enlace
LDFLAGS = -lm

# Archivos fuente
SRC = src/main.c src/mandelbrot.c src/convolution.c src/histogram.c src/image_io.c

# Directorio de binarios
BIN_DIR = bin

# Detección de extensión en Windows
ifeq ($(OS),Windows_NT)
    EXE_EXT = .exe
    MKDIR = if not exist $(subst /,\,$1) mkdir $(subst /,\,$1)
    RM = del /Q /S
    RMDIR = rmdir /Q /S
else
    EXE_EXT =
    MKDIR = mkdir -p $1
    RM = rm -f
    RMDIR = rm -rf
endif

# Nombres de los binarios
TARGET_SEQ = $(BIN_DIR)/fractal_seq$(EXE_EXT)
TARGET_PAR = $(BIN_DIR)/fractal_par$(EXE_EXT)
TARGET_VEC = $(BIN_DIR)/fractal_vec$(EXE_EXT)

# ==============================================================================
# Reglas principales
# ==============================================================================

# Por defecto: compilar versión paralela
.DEFAULT_GOAL := parallel

# Compilar todas las versiones
all: dirs sequential parallel vectorized

# Versión secuencial (sin OpenMP)
sequential: dirs
	@echo "=== Compilando version secuencial ==="
	$(CC) $(CFLAGS_SEQ) $(SRC) -o $(TARGET_SEQ) $(LDFLAGS)
	@echo "=== Binario generado: $(TARGET_SEQ) ==="

# Versión paralela (con OpenMP)
parallel: dirs
	@echo "=== Compilando version paralela (OpenMP) ==="
	$(CC) $(CFLAGS_PAR) $(SRC) -o $(TARGET_PAR) $(LDFLAGS)
	@echo "=== Binario generado: $(TARGET_PAR) ==="

# Versión vectorizada (OpenMP + SIMD nativo)
vectorized: dirs
	@echo "=== Compilando version vectorizada (OpenMP + SIMD) ==="
	$(CC) $(CFLAGS_VEC) $(SRC) -o $(TARGET_VEC) $(LDFLAGS)
	@echo "=== Binario generado: $(TARGET_VEC) ==="

# ==============================================================================
# Directorios
# ==============================================================================

dirs:
ifeq ($(OS),Windows_NT)
	@if not exist bin mkdir bin
	@if not exist output mkdir output
	@if not exist results mkdir results
else
	@mkdir -p bin output results
endif

# ==============================================================================
# Limpieza
# ==============================================================================

clean:
ifeq ($(OS),Windows_NT)
	@if exist bin rmdir /Q /S bin
	@if exist output\*.ppm del /Q output\*.ppm
	@echo === Limpieza completada ===
else
	@rm -rf bin/
	@rm -f output/*.ppm
	@echo "=== Limpieza completada ==="
endif

# ==============================================================================
# Ayuda
# ==============================================================================

help:
	@echo ""
	@echo "Targets disponibles:"
	@echo "  make              - Compilar version paralela (por defecto)"
	@echo "  make all          - Compilar todas las versiones"
	@echo "  make sequential   - Compilar version secuencial"
	@echo "  make parallel     - Compilar version paralela (OpenMP)"
	@echo "  make vectorized   - Compilar version vectorizada (OpenMP + SIMD)"
	@echo "  make clean        - Eliminar binarios y archivos generados"
	@echo "  make dirs         - Crear directorios necesarios"
	@echo "  make help         - Mostrar esta ayuda"
	@echo ""

.PHONY: all sequential parallel vectorized clean dirs help
