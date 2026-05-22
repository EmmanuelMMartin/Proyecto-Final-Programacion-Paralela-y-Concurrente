# Proyecto de Programación Paralela — Fractal Mandelbrot + Filtro de Convolución con OpenMP

## Descripción

Este proyecto implementa la generación de fractales del conjunto de Mandelbrot en resolución 8K (7680×4320) y la aplicación de un filtro de convolución Gaussiana, utilizando paralelización con **OpenMP** en C. El objetivo es analizar el rendimiento de distintas estrategias de paralelización, incluyendo planificadores de hilos, modos de sincronización para histogramas, vectorización SIMD y afinidad de hilos.

### Características principales

- **Generación del fractal de Mandelbrot** con iteraciones configurables y paleta de colores suave.
- **Filtro de convolución Gaussiana** aplicado a la imagen generada.
- **Histograma de intensidades** con múltiples implementaciones: secuencial, atómica, con sección crítica, reducción y *false sharing* intencional.
- **Benchmarking automatizado** con variación de hilos, planificadores, tamaños de *chunk* y modos de afinidad.
- **Gráficas de resultados** generadas con Python (matplotlib).

---

## Prerrequisitos

### Compilador C

- **GCC** 9.0 o superior con soporte para OpenMP
- En Windows: se recomienda [MSYS2](https://www.msys2.org/) o [MinGW-w64](https://www.mingw-w64.org/)
- En Linux: `sudo apt install gcc` (Ubuntu/Debian) o equivalente

### Python 3 (para gráficas)

- Python 3.8 o superior
- Bibliotecas requeridas:

```bash
pip install matplotlib numpy pandas
```

### Herramientas adicionales

- `make` (GNU Make)
- `bash` (para los scripts de benchmarking)

---

## Compilación

El proyecto utiliza un `Makefile` con tres modos de compilación:

```bash
# Compilar la versión paralela (por defecto)
make

# Compilar solo la versión secuencial
make sequential

# Compilar la versión paralela con OpenMP
make parallel

# Compilar la versión vectorizada (SIMD + OpenMP)
make vectorized

# Compilar todas las versiones
make all

# Limpiar archivos generados
make clean
```

Los binarios se generan en el directorio `bin/`:

| Versión      | Binario               |
|------------- |---------------------- |
| Secuencial   | `bin/fractal_seq`     |
| Paralela     | `bin/fractal_par`     |
| Vectorizada  | `bin/fractal_vec`     |

---

## Ejecución

### Uso básico

```bash
# Generar fractal con configuración por defecto (8K, versión paralela)
./bin/fractal_par

# Generar fractal con resolución personalizada
./bin/fractal_par --width 1920 --height 1080

# Especificar número de hilos
./bin/fractal_par --threads 8

# Usar planificador específico
./bin/fractal_par --threads 8 --scheduler dynamic --chunk 100

# Seleccionar modo de histograma
./bin/fractal_par --threads 8 --histogram atomic

# Generar salida con nombre personalizado
./bin/fractal_par --output output/mi_fractal.ppm
```

### Argumentos de línea de comandos

| Argumento        | Descripción                                      | Valor por defecto  |
|----------------- |------------------------------------------------- |------------------- |
| `--width`        | Ancho de la imagen en píxeles                    | `7680`             |
| `--height`       | Alto de la imagen en píxeles                     | `4320`             |
| `--max-iter`     | Número máximo de iteraciones de Mandelbrot       | `1000`             |
| `--threads`      | Número de hilos OpenMP                           | Automático (cores) |
| `--scheduler`    | Planificador OpenMP: `static`, `dynamic`, `guided` | `static`         |
| `--chunk`        | Tamaño del chunk para el planificador            | `0` (auto)         |
| `--histogram`    | Modo de histograma: `sequential`, `atomic`, `critical`, `reduction`, `false_sharing` | `reduction` |
| `--output`       | Ruta del archivo de salida (formato PPM)         | `output/mandelbrot.ppm` |
| `--no-convolution` | Deshabilitar el filtro de convolución          | Habilitado         |
| `--kernel-size`  | Tamaño del kernel Gaussiano (debe ser impar)     | `5`                |
| `--sigma`        | Desviación estándar del kernel Gaussiano         | `1.0`              |
| `--help`         | Mostrar ayuda                                    | —                  |

---

## Ejecución de benchmarks

```bash
# Ejecutar todos los benchmarks
bash scripts/run_benchmarks.sh

# Ejecutar pruebas de afinidad
bash scripts/run_affinity_tests.sh

# Generar gráficas de resultados
python scripts/plot_results.py
```

Los resultados se guardan en formato CSV en `results/` y las gráficas como PNG en el mismo directorio.

---

## Estructura del proyecto

```
Proyecto_Progra_Paralela/
├── README.md                          # Este archivo
├── Makefile                           # Sistema de compilación
├── .gitignore                         # Archivos ignorados por Git
│
├── src/                               # Código fuente en C
│   ├── main.c                         # Punto de entrada y manejo de argumentos
│   ├── mandelbrot.c                   # Generación del fractal de Mandelbrot
│   ├── convolution.c                  # Filtro de convolución Gaussiana
│   ├── histogram.c                    # Implementaciones de histograma
│   ├── image_io.c                     # Lectura/escritura de imágenes PPM
│   ├── mandelbrot.h                   # Header de mandelbrot
│   ├── convolution.h                  # Header de convolución
│   ├── histogram.h                    # Header de histograma
│   └── image_io.h                     # Header de E/S de imágenes
│
├── scripts/                           # Scripts de automatización
│   ├── run_benchmarks.sh              # Benchmarks de rendimiento
│   ├── run_affinity_tests.sh          # Pruebas de afinidad de hilos
│   └── plot_results.py                # Generación de gráficas
│
├── docs/                              # Documentación
│   ├── hardware_specs.md              # Especificaciones del hardware
│   ├── prompts_utilizados.md          # Prompts usados con IA
│   └── reporte_tecnico.md             # Reporte técnico completo
│
├── output/                            # Imágenes generadas (ignorado por Git)
│   └── mandelbrot.ppm
│
├── results/                           # Resultados de benchmarks y gráficas
│   ├── benchmark_threads.csv
│   ├── benchmark_schedulers.csv
│   ├── benchmark_histogram.csv
│   ├── benchmark_affinity.csv
│   └── *.png                          # Gráficas generadas
│
└── bin/                               # Binarios compilados (ignorado por Git)
    ├── fractal_seq
    ├── fractal_par
    └── fractal_vec
```

---

## Autores

| Nombre                | Carné / ID   | Rol                          |
|---------------------- |------------- |----------------------------- |
| *[Nombre Completo 1]* | *[Carné]*   | Desarrollo e implementación  |
| *[Nombre Completo 2]* | *[Carné]*   | Benchmarking y análisis      |
| *[Nombre Completo 3]* | *[Carné]*   | Documentación y reporte      |

**Curso:** Programación Paralela y Concurrente  
**Universidad:** *[Nombre de la Universidad]*  
**Fecha:** Mayo 2026

---

## Licencia

Este proyecto fue desarrollado con fines académicos. Todos los derechos reservados.

Se permite el uso y modificación del código con fines educativos, siempre que se otorgue la atribución correspondiente a los autores originales.

---

## Referencias

- [OpenMP Specification](https://www.openmp.org/specifications/)
- [Mandelbrot Set — Wikipedia](https://en.wikipedia.org/wiki/Mandelbrot_set)
- [Gaussian Blur — Wikipedia](https://en.wikipedia.org/wiki/Gaussian_blur)
- Amdahl, G. M. (1967). *Validity of the single processor approach to achieving large scale computing capabilities.*
