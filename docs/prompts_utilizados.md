# Prompts Utilizados y Análisis del Código Generado por IA

> **Instrucciones:** Documente cada prompt utilizado con la herramienta de IA, junto con un análisis crítico del código generado. Esto es esencial para evaluar la calidad de la asistencia de IA en programación paralela.

---

## Índice

1. [Prompt 1: Generación del código secuencial base](#prompt-1-generación-del-código-secuencial-base)
2. [Prompt 2: Paralelización con OpenMP](#prompt-2-paralelización-con-openmp)
3. [Prompt 3: Optimización y vectorización](#prompt-3-optimización-y-vectorización)
4. [Análisis General del Código Generado por IA](#análisis-general-del-código-generado-por-ia)

---

## Prompt 1: Generación del código secuencial base

### Prompt utilizado

```
[Pegue aquí el prompt exacto que utilizó para generar el código secuencial base]
```

### Herramienta de IA utilizada

- **Modelo:** *[Ej: ChatGPT-4, Claude 3.5, Gemini, etc.]*
- **Fecha:** *[Ej: 2026-05-XX]*

### Código generado

*[Resuma brevemente qué archivos y funciones generó la IA]*

### Análisis del código generado

#### ✅ Aspectos positivos
- *[Ej: Estructura modular correcta, separación de responsabilidades]*
- *[Ej: Uso correcto del algoritmo de escape de Mandelbrot]*
- *[Ej: Formato PPM implementado correctamente]*

#### ⚠️ Problemas encontrados

| #  | Problema                    | Severidad | Descripción                                          |
|--- |---------------------------- |---------- |----------------------------------------------------- |
| 1  | *[Ej: Desbordamiento]*     | Alta      | *[Descripción del problema]*                         |
| 2  | *[Ej: Ineficiencia]*       | Media     | *[Descripción del problema]*                         |
| 3  | *[Ej: Error lógico]*       | Alta      | *[Descripción del problema]*                         |

#### 🔧 Correcciones realizadas

```c
// Ejemplo de corrección aplicada
// ANTES (código IA):
// [código original de la IA]

// DESPUÉS (código corregido):
// [código corregido manualmente]
```

---

## Prompt 2: Paralelización con OpenMP

### Prompt utilizado

```
[Pegue aquí el prompt exacto que utilizó para paralelizar el código con OpenMP]
```

### Herramienta de IA utilizada

- **Modelo:** *[Ej: ChatGPT-4, Claude 3.5, Gemini, etc.]*
- **Fecha:** *[Ej: 2026-05-XX]*

### Código generado

*[Resuma brevemente las directivas OpenMP y estrategias de paralelización generadas]*

### Análisis del código generado

#### ✅ Aspectos positivos
- *[Ej: Uso correcto de #pragma omp parallel for]*
- *[Ej: Identificación de variables privadas vs compartidas]*

#### ⚠️ Problemas encontrados

| #  | Problema                          | Severidad | Descripción                                 |
|--- |---------------------------------- |---------- |-------------------------------------------- |
| 1  | *[Ej: Condición de carrera]*      | Crítica   | *[Descripción del data race]*               |
| 2  | *[Ej: False sharing]*            | Alta      | *[Descripción del false sharing]*           |
| 3  | *[Ej: Overhead excesivo]*        | Media     | *[Descripción del overhead]*                |
| 4  | *[Ej: Scheduler inapropiado]*    | Baja      | *[Descripción del problema]*                |

#### 🔧 Correcciones realizadas

```c
// Ejemplo de corrección de condición de carrera
// ANTES (código IA):
// #pragma omp parallel for
// for (int i = 0; i < N; i++) {
//     histogram[pixel_value]++;  // ← DATA RACE
// }

// DESPUÉS (código corregido):
// #pragma omp parallel for reduction(+:histogram[:256])
// for (int i = 0; i < N; i++) {
//     histogram[pixel_value]++;  // ← Correcto con reducción
// }
```

#### 🔍 Análisis de cuellos de botella

- **Desbalance de carga:** *[¿La IA consideró el desbalance en Mandelbrot?]*
- **Sincronización:** *[¿Qué primitivas de sincronización sugirió?]*
- **Granularidad:** *[¿El nivel de paralelismo es adecuado?]*

---

## Prompt 3: Optimización y vectorización

### Prompt utilizado

```
[Pegue aquí el prompt exacto utilizado para optimización y vectorización]
```

### Herramienta de IA utilizada

- **Modelo:** *[Ej: ChatGPT-4, Claude 3.5, Gemini, etc.]*
- **Fecha:** *[Ej: 2026-05-XX]*

### Análisis del código generado

#### ✅ Aspectos positivos
- *[Ej: Uso de pragma omp simd]*
- *[Ej: Alineación de memoria para vectorización]*

#### ⚠️ Problemas encontrados

| #  | Problema                    | Severidad | Descripción                                    |
|--- |---------------------------- |---------- |----------------------------------------------- |
| 1  | *[Problema]*                | *[Sev]*   | *[Descripción]*                                |

---

## Análisis General del Código Generado por IA

### Resumen cuantitativo

| Métrica                             | Valor                |
|------------------------------------ |--------------------- |
| Total de prompts utilizados         | *[Número]*           |
| Líneas de código generadas por IA   | *[Número]*           |
| Líneas modificadas manualmente      | *[Número]*           |
| Porcentaje de código IA utilizable   | *[Ej: 75%]*         |
| Errores críticos encontrados        | *[Número]*           |
| Errores de rendimiento encontrados  | *[Número]*           |

### Patrones observados en el código de IA

1. **Tendencia a ignorar condiciones de carrera:**
   *[Describa si la IA frecuentemente generó código con data races]*

2. **Elección de planificadores:**
   *[¿Qué planificador sugirió la IA? ¿Era apropiado para Mandelbrot?]*

3. **Manejo de false sharing:**
   *[¿La IA consideró false sharing en sus implementaciones?]*

4. **Vectorización:**
   *[¿La IA generó código vectorizable? ¿Usó pragma omp simd?]*

### Conclusiones sobre el uso de IA

*[Reflexione sobre la utilidad y limitaciones de la IA para programación paralela:]*

- *¿Qué tan útil fue la IA como punto de partida?*
- *¿Qué tipos de errores son más comunes en código paralelo generado por IA?*
- *¿Qué conocimientos previos son necesarios para validar el código de IA?*
- *¿Recomendaría usar IA para programación paralela? ¿Con qué precauciones?*

---

## Historial de Prompts Adicionales

*Si utilizó prompts adicionales, documéntelos aquí siguiendo el mismo formato.*

| #  | Prompt (resumen)                | Resultado                    | Calidad |
|--- |-------------------------------- |----------------------------- |-------- |
| 4  | *[Resumen del prompt]*          | *[Qué generó]*               | *[⭐⭐⭐]* |
| 5  | *[Resumen del prompt]*          | *[Qué generó]*               | *[⭐⭐]*  |
