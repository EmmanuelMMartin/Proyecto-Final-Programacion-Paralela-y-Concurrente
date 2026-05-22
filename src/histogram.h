/**
 * ============================================================================
 * histogram.h - Cálculo de histogramas de color con múltiples estrategias
 * ============================================================================
 * 
 * Proporciona funciones para calcular histogramas RGB de una imagen
 * utilizando diferentes estrategias de paralelización OpenMP:
 * 
 *   1. Secuencial:     Sin OpenMP, referencia de corrección.
 *   2. Atomic:         Usa #pragma omp atomic para actualizar contadores.
 *   3. Critical:       Usa #pragma omp critical (alta contención).
 *   4. Reduction:      Arreglos locales por hilo + reducción manual (óptima).
 *   5. False Sharing:  Deliberadamente mala, demuestra el efecto de
 *                      compartición falsa (false sharing) de caché.
 * 
 * Cada histograma tiene 256 entradas (HIST_SIZE) por canal (R, G, B).
 * 
 * Autor: Proyecto Programación Paralela
 * ============================================================================
 */

#ifndef HISTOGRAM_H
#define HISTOGRAM_H

#include "common.h"

/** Tamaño del histograma: 256 niveles por canal */
#define HIST_SIZE 256

/**
 * compute_histogram_sequential - Calcula histograma sin paralelización.
 * 
 * Versión de referencia para verificar corrección y medir speedup.
 * 
 * @param img    Imagen de entrada.
 * @param hist_r Arreglo de salida para el histograma del canal rojo.
 * @param hist_g Arreglo de salida para el histograma del canal verde.
 * @param hist_b Arreglo de salida para el histograma del canal azul.
 */
void compute_histogram_sequential(const Image *img,
                                  long long hist_r[HIST_SIZE],
                                  long long hist_g[HIST_SIZE],
                                  long long hist_b[HIST_SIZE]);

/**
 * compute_histogram_atomic - Histograma paralelo con #pragma omp atomic.
 * 
 * Cada hilo actualiza directamente los arreglos compartidos usando
 * operaciones atómicas. Moderada contención en caché.
 * 
 * @param img    Imagen de entrada.
 * @param hist_r Arreglo de salida para el histograma del canal rojo.
 * @param hist_g Arreglo de salida para el histograma del canal verde.
 * @param hist_b Arreglo de salida para el histograma del canal azul.
 */
void compute_histogram_atomic(const Image *img,
                              long long hist_r[HIST_SIZE],
                              long long hist_g[HIST_SIZE],
                              long long hist_b[HIST_SIZE]);

/**
 * compute_histogram_critical - Histograma paralelo con #pragma omp critical.
 * 
 * Usa una sección crítica para proteger las actualizaciones.
 * Alta contención: los hilos se serializan en la sección crítica.
 * 
 * @param img    Imagen de entrada.
 * @param hist_r Arreglo de salida para el histograma del canal rojo.
 * @param hist_g Arreglo de salida para el histograma del canal verde.
 * @param hist_b Arreglo de salida para el histograma del canal azul.
 */
void compute_histogram_critical(const Image *img,
                                long long hist_r[HIST_SIZE],
                                long long hist_g[HIST_SIZE],
                                long long hist_b[HIST_SIZE]);

/**
 * compute_histogram_reduction - Histograma paralelo con reducción manual.
 * 
 * Cada hilo mantiene arreglos locales que se combinan al final.
 * Esta es la estrategia óptima: evita contención y compartición falsa.
 * 
 * @param img    Imagen de entrada.
 * @param hist_r Arreglo de salida para el histograma del canal rojo.
 * @param hist_g Arreglo de salida para el histograma del canal verde.
 * @param hist_b Arreglo de salida para el histograma del canal azul.
 */
void compute_histogram_reduction(const Image *img,
                                 long long hist_r[HIST_SIZE],
                                 long long hist_g[HIST_SIZE],
                                 long long hist_b[HIST_SIZE]);

/**
 * compute_histogram_false_sharing - Histograma con compartición falsa deliberada.
 * 
 * ¡VERSIÓN INTENCIONALMENTE INEFICIENTE!
 * 
 * Cada hilo escribe en posiciones adyacentes de un arreglo compartido,
 * causando invalidaciones constantes de líneas de caché (false sharing).
 * Se incluye para demostrar el impacto negativo en rendimiento.
 * 
 * @param img    Imagen de entrada.
 * @param hist_r Arreglo de salida para el histograma del canal rojo.
 * @param hist_g Arreglo de salida para el histograma del canal verde.
 * @param hist_b Arreglo de salida para el histograma del canal azul.
 */
void compute_histogram_false_sharing(const Image *img,
                                     long long hist_r[HIST_SIZE],
                                     long long hist_g[HIST_SIZE],
                                     long long hist_b[HIST_SIZE]);

/**
 * print_histogram_summary - Imprime un resumen estadístico del histograma.
 * 
 * Muestra: total de pixeles, valor medio, valor más frecuente (moda),
 * y distribución general por canal.
 * 
 * @param hist_r Histograma del canal rojo.
 * @param hist_g Histograma del canal verde.
 * @param hist_b Histograma del canal azul.
 */
void print_histogram_summary(const long long hist_r[HIST_SIZE],
                             const long long hist_g[HIST_SIZE],
                             const long long hist_b[HIST_SIZE]);

#endif /* HISTOGRAM_H */
