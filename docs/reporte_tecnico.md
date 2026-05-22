# Reporte Técnico — Fractal Mandelbrot + Filtro de Convolución con OpenMP

**Curso:** Programación Paralela y Concurrente  
**Universidad:** *[Nombre de la Universidad]*  
**Fecha:** Mayo 2026  
**Autores:**
- *[Nombre Completo 1]* — *[Carné]*
- *[Nombre Completo 2]* — *[Carné]*
- *[Nombre Completo 3]* — *[Carné]*

---

## Tabla de Contenidos

1. [Introducción](#1-introducción)
2. [Especificaciones del Hardware](#2-especificaciones-del-hardware)
3. [Metodología](#3-metodología)
   - 3.1 [Prompts y Generación de Código con IA](#31-prompts-y-generación-de-código-con-ia)
   - 3.2 [Análisis del Código Generado por IA](#32-análisis-del-código-generado-por-ia)
   - 3.3 [Diseño Experimental](#33-diseño-experimental)
4. [Resultados](#4-resultados)
   - 4.1 [Línea Base Secuencial](#41-línea-base-secuencial)
   - 4.2 [Línea Base Paralela (Código IA)](#42-línea-base-paralela-código-ia)
   - 4.3 [Evaluación de Schedulers](#43-evaluación-de-schedulers)
   - 4.4 [Sincronización y False Sharing](#44-sincronización-y-false-sharing)
   - 4.5 [Vectorización SPMD](#45-vectorización-spmd)
   - 4.6 [Afinidad de Hilos](#46-afinidad-de-hilos)
5. [Análisis de Rendimiento](#5-análisis-de-rendimiento)
   - 5.1 [Ley de Amdahl](#51-ley-de-amdahl)
   - 5.2 [Análisis de Overhead](#52-análisis-de-overhead)
   - 5.3 [Eficiencia Paralela](#53-eficiencia-paralela)
6. [Conclusiones](#6-conclusiones)
7. [Referencias](#7-referencias)

---

## 1. Introducción

### 1.1 Contexto

La programación paralela es fundamental para aprovechar al máximo los procesadores multinúcleo modernos. Este proyecto explora la paralelización de dos problemas computacionalmente intensivos:

1. **Generación del fractal de Mandelbrot:** Un problema *embarrassingly parallel* con desbalance de carga inherente, donde las regiones dentro del conjunto requieren el máximo de iteraciones mientras que las regiones externas convergen rápidamente.

2. **Filtro de convolución Gaussiana:** Una operación de procesamiento de imágenes que aplica un kernel de convolución a cada píxel, con patrón de acceso a memoria regular y alta localidad espacial.

### 1.2 Objetivos

- Implementar versiones secuencial y paralela (OpenMP) de ambos algoritmos.
- Evaluar el impacto de diferentes planificadores de OpenMP en el rendimiento.
- Analizar el efecto de la sincronización y el *false sharing* en la implementación de histogramas.
- Explorar la vectorización SIMD como técnica de optimización complementaria.
- Estudiar el impacto de la afinidad de hilos en el rendimiento.
- Aplicar la Ley de Amdahl para modelar los límites teóricos del speedup.

### 1.3 Alcance

El proyecto genera imágenes en resolución 8K (7680×4320 píxeles) con hasta 1000 iteraciones de Mandelbrot y aplica un filtro Gaussiano de tamaño configurable. Los benchmarks se ejecutan con resolución reducida (1920×1080) para eficiencia.

---

## 2. Especificaciones del Hardware

> **Nota:** Consulte el archivo `docs/hardware_specs.md` para las especificaciones detalladas y cómo obtenerlas.

| Componente              | Especificación               |
|------------------------ |----------------------------- |
| **CPU**                 | *[Modelo del procesador]*    |
| **Núcleos / Hilos**     | *[Ej: 8 / 16]*              |
| **Frecuencia**          | *[Ej: 3.6 GHz base]*        |
| **Caché L1/L2/L3**      | *[Ej: 32K / 256K / 16MB]*  |
| **RAM**                 | *[Ej: 16 GB DDR4 3200]*     |
| **OS**                  | *[Ej: Windows 11]*          |
| **Compilador**          | *[Ej: GCC 13.2.0]*          |
| **Banderas de compilación** | `-O2 -fopenmp -std=c11` |

---

## 3. Metodología

### 3.1 Prompts y Generación de Código con IA

> **Detalle completo:** Consulte `docs/prompts_utilizados.md`

Se utilizó *[herramienta de IA]* para generar el código base. Se emplearon los siguientes prompts principales:

1. **Prompt 1 — Código secuencial base:**
   *[Resuma brevemente el prompt]*

2. **Prompt 2 — Paralelización con OpenMP:**
   *[Resuma brevemente el prompt]*

3. **Prompt 3 — Optimización:**
   *[Resuma brevemente el prompt]*

### 3.2 Análisis del Código Generado por IA

#### Errores y problemas identificados

*[Describa los principales errores encontrados en el código generado por IA]*

| Error                        | Tipo          | Impacto en rendimiento | Corrección aplicada        |
|----------------------------- |-------------- |----------------------- |--------------------------- |
| *[Ej: Data race en histograma]* | Correctitud | N/A (resultado incorrecto) | *[Uso de reducción]*   |
| *[Ej: Scheduler incorrecto]*    | Rendimiento | *[Ej: -30% speedup]*   | *[Cambio a dynamic]*      |

#### Cuellos de botella identificados

1. *[Ej: Desbalance de carga en Mandelbrot con scheduler static]*
2. *[Ej: False sharing en histograma con arrays compartidos]*
3. *[Ej: Overhead de sincronización con secciones críticas]*

### 3.3 Diseño Experimental

#### Variables independientes

| Variable                | Valores evaluados                                        |
|------------------------ |--------------------------------------------------------- |
| Número de hilos         | 1, 2, 4, 6, 8, 10, 12, 14, 16, 20, 24, 32             |
| Planificador OpenMP     | static, dynamic, guided                                  |
| Tamaño de chunk         | 1, 10, 50, 100, 500                                     |
| Modo de histograma      | sequential, atomic, critical, reduction, false_sharing   |
| Afinidad (PROC_BIND)   | close, spread, master                                    |
| Afinidad (PLACES)      | cores, threads, sockets                                  |

#### Variables dependientes

- Tiempo de ejecución de Mandelbrot (s)
- Tiempo de ejecución de convolución (s)
- Tiempo de histograma (s)
- Tiempo total (s)
- Speedup (T₁/Tₙ)
- Eficiencia (Speedup/n)

#### Condiciones de prueba

- Resolución de benchmark: 1920×1080
- Iteraciones máximas: 500
- Repeticiones por configuración: 3 (promediadas)
- Sistema en modo de alto rendimiento
- Procesos no esenciales minimizados

---

## 4. Resultados

### 4.1 Línea Base Secuencial

*[Complete con los datos obtenidos]*

| Componente        | Tiempo (s)    |
|------------------ |-------------- |
| Mandelbrot        | *[valor]*     |
| Convolución       | *[valor]*     |
| Histograma        | *[valor]*     |
| **Total**         | *[valor]*     |

**Observaciones:**
- *[Ej: El cálculo de Mandelbrot domina el tiempo total (~80%)]*
- *[Ej: La convolución muestra buen acceso a caché]*

### 4.2 Línea Base Paralela (Código IA)

*[Resultados del código paralelo generado por IA sin modificaciones]*

| Hilos | Tiempo Total (s) | Speedup | Eficiencia |
|------ |------------------ |-------- |----------- |
| 1     | *[valor]*         | 1.00x   | 100%       |
| 2     | *[valor]*         | *[S]*   | *[E]*      |
| 4     | *[valor]*         | *[S]*   | *[E]*      |
| 8     | *[valor]*         | *[S]*   | *[E]*      |
| 16    | *[valor]*         | *[S]*   | *[E]*      |

*[Insertar gráfica de execution_time aquí]*

**Análisis:**
- *[¿El speedup es sublineal, lineal o superlineal?]*
- *[¿A partir de cuántos hilos se estanca?]*
- *[¿Hay degradación con muchos hilos?]*

### 4.3 Evaluación de Schedulers

*[Tabla comparativa de planificadores]*

| Planificador | Chunk | Mandelbrot (s) | Convolución (s) | Total (s) |
|------------- |------ |---------------- |----------------- |---------- |
| static       | 1     | *[valor]*       | *[valor]*        | *[valor]* |
| static       | 10    | *[valor]*       | *[valor]*        | *[valor]* |
| dynamic      | 1     | *[valor]*       | *[valor]*        | *[valor]* |
| dynamic      | 10    | *[valor]*       | *[valor]*        | *[valor]* |
| guided       | 1     | *[valor]*       | *[valor]*        | *[valor]* |
| guided       | 10    | *[valor]*       | *[valor]*        | *[valor]* |

*[Insertar gráfica de schedulers aquí]*

**Análisis:**
- *[¿Cuál planificador es mejor para Mandelbrot? ¿Por qué?]*
- *[¿Cuál es mejor para convolución? ¿Por qué?]*
- *[¿Cómo afecta el tamaño del chunk?]*
- *[¿Hay overhead significativo con dynamic y chunk pequeño?]*

### 4.4 Sincronización y False Sharing

*[Comparación de modos de histograma]*

| Modo           | Hilos | Tiempo Histograma (s) | Observación                          |
|--------------- |------ |---------------------- |------------------------------------- |
| sequential     | 1     | *[valor]*             | Referencia                           |
| atomic         | 8     | *[valor]*             | *[¿Mejor/peor que secuencial?]*      |
| critical       | 8     | *[valor]*             | *[¿Cuánto overhead?]*                |
| reduction      | 8     | *[valor]*             | *[¿Es el más rápido?]*               |
| false_sharing  | 8     | *[valor]*             | *[¿Cuánta degradación?]*             |

*[Insertar gráfica de histogram aquí]*

**Análisis:**
- *[Explique por qué `reduction` es generalmente más rápido]*
- *[Explique por qué `critical` tiene más overhead]*
- *[Explique el fenómeno de false sharing y su impacto]*
- *[¿El modo atómico es viable para histogramas?]*

### 4.5 Vectorización SPMD

*[Comparación entre versión paralela y vectorizada]*

| Versión      | Mandelbrot (s) | Convolución (s) | Total (s) | Speedup vs Paralela |
|------------- |---------------- |----------------- |---------- |-------------------- |
| Paralela     | *[valor]*       | *[valor]*        | *[valor]* | 1.00x               |
| Vectorizada  | *[valor]*       | *[valor]*        | *[valor]* | *[valor]*           |

**Análisis:**
- *[¿La vectorización mejoró el rendimiento?]*
- *[¿Qué loops se vectorizaron exitosamente? (ver salida de -fopt-info-vec)]*
- *[¿Mandelbrot es fácil/difícil de vectorizar? ¿Por qué?]*
- *[¿Convolución se beneficia más de SIMD? ¿Por qué?]*

### 4.6 Afinidad de Hilos

*[Resultados de las pruebas de afinidad]*

| PROC_BIND | PLACES  | Tiempo Total (s) | Δ vs Base (%) |
|---------- |-------- |------------------ |-------------- |
| (ninguno) | (ninguno) | *[valor]*       | —              |
| close     | cores   | *[valor]*         | *[%]*          |
| close     | threads | *[valor]*         | *[%]*          |
| spread    | cores   | *[valor]*         | *[%]*          |
| spread    | threads | *[valor]*         | *[%]*          |
| master    | cores   | *[valor]*         | *[%]*          |

*[Insertar gráfica de affinity aquí]*

**Análisis:**
- *[¿Cuál configuración de afinidad fue óptima?]*
- *[¿Close o spread funciona mejor? ¿Por qué?]*
- *[¿La afinidad tiene un impacto significativo en este problema?]*

---

## 5. Análisis de Rendimiento

### 5.1 Ley de Amdahl

La Ley de Amdahl establece el límite teórico del speedup:

$$S(n) = \frac{1}{(1-p) + \frac{p}{n}}$$

Donde:
- $S(n)$ = speedup con $n$ procesadores
- $p$ = fracción paralelizable del programa
- $n$ = número de procesadores

#### Estimación de la fracción paralela

*[Calcule p a partir de los datos experimentales]*

| Método de estimación       | Fracción paralela (p) |
|--------------------------- |---------------------- |
| Medición directa (perfiles) | *[valor]*            |
| Regresión de speedup       | *[valor]*            |

#### Speedup máximo teórico

Con $p$ = *[valor]*:

| Hilos (n) | Speedup teórico (Amdahl) | Speedup experimental | Eficiencia |
|---------- |-------------------------- |--------------------- |----------- |
| 2         | *[valor]*                 | *[valor]*            | *[%]*      |
| 4         | *[valor]*                 | *[valor]*            | *[%]*      |
| 8         | *[valor]*                 | *[valor]*            | *[%]*      |
| 16        | *[valor]*                 | *[valor]*            | *[%]*      |
| ∞         | *[1/(1-p)]*               | —                    | —          |

*[Insertar gráfica de speedup con curva de Amdahl aquí]*

### 5.2 Análisis de Overhead

*[Identifique y cuantifique las fuentes de overhead]*

| Fuente de overhead              | Impacto estimado |
|-------------------------------- |----------------- |
| Creación/destrucción de hilos   | *[valor]*        |
| Sincronización (barriers)       | *[valor]*        |
| Planificación dinámica          | *[valor]*        |
| False sharing                   | *[valor]*        |
| Desbalance de carga residual    | *[valor]*        |

### 5.3 Eficiencia Paralela

La eficiencia paralela se define como:

$$E(n) = \frac{S(n)}{n} = \frac{T_1}{n \cdot T_n}$$

*[Incluir tabla o gráfica de eficiencia]*

| Hilos | Eficiencia (%) | Clasificación            |
|------ |---------------- |------------------------- |
| 2     | *[valor]*       | *[Excelente/Buena/Baja]* |
| 4     | *[valor]*       | *[Excelente/Buena/Baja]* |
| 8     | *[valor]*       | *[Excelente/Buena/Baja]* |
| 16    | *[valor]*       | *[Excelente/Buena/Baja]* |

---

## 6. Conclusiones

### 6.1 Hallazgos principales

1. **Speedup alcanzado:** *[Resuma el speedup máximo obtenido y con cuántos hilos]*

2. **Mejor planificador:** *[Identifique cuál planificador funciona mejor para cada kernel]*

3. **False sharing:** *[Describa el impacto observado y la importancia de evitarlo]*

4. **Vectorización:** *[Resuma si la vectorización aportó beneficios significativos]*

5. **Afinidad:** *[Indique si la afinidad de hilos tuvo impacto significativo]*

### 6.2 Lecciones aprendidas

- *[Lección 1 sobre programación paralela]*
- *[Lección 2 sobre uso de IA para código paralelo]*
- *[Lección 3 sobre benchmarking y análisis]*

### 6.3 Trabajo futuro

- *[Ej: Implementación con GPU (CUDA/OpenCL)]*
- *[Ej: Distribución con MPI]*
- *[Ej: Optimización de acceso a memoria (tiling)]*
- *[Ej: Paralelismo de tareas con OpenMP tasks]*

---

## 7. Referencias

1. OpenMP Architecture Review Board. (2021). *OpenMP Application Programming Interface Specification, Version 5.2*. https://www.openmp.org/specifications/

2. Amdahl, G. M. (1967). Validity of the single processor approach to achieving large scale computing capabilities. *AFIPS Conference Proceedings*, 30, 483-485.

3. Chapman, B., Jost, G., & Van Der Pas, R. (2007). *Using OpenMP: Portable Shared Memory Parallel Programming*. MIT Press.

4. Mandelbrot, B. B. (1980). Fractal aspects of the iteration of $z \to \lambda z(1-z)$ for complex $\lambda$ and $z$. *Annals of the New York Academy of Sciences*, 357(1), 249-259.

5. Gonzalez, R. C., & Woods, R. E. (2018). *Digital Image Processing* (4th ed.). Pearson.

6. *[Agregue referencias adicionales utilizadas]*

---

> **Nota:** Este documento es una plantilla/esqueleto. Complete cada sección con los datos experimentales obtenidos al ejecutar los benchmarks del proyecto.
