/**
 * ============================================================================
 * mandelbrot.h - Generación del fractal de Mandelbrot
 * ============================================================================
 * 
 * Proporciona funciones para generar el conjunto de Mandelbrot con
 * coloración suave basada en escape time continuo y conversión HSV a RGB.
 * 
 * Versiones disponibles:
 *   - Secuencial: sin pragmas OpenMP
 *   - Paralela: con #pragma omp parallel for schedule(runtime)
 * 
 * Vista por defecto del plano complejo:
 *   x ∈ [-2.5, 1.0], y ∈ [-1.25, 1.25]
 * 
 * Autor: Proyecto Programación Paralela
 * ============================================================================
 */

#ifndef MANDELBROT_H
#define MANDELBROT_H

#include "common.h"

/**
 * generate_mandelbrot - Genera el fractal de Mandelbrot (versión secuencial).
 * 
 * Calcula el conjunto de Mandelbrot para la región del plano complejo
 * especificada y almacena el resultado como imagen RGB usando coloración
 * suave con paleta HSV cíclica.
 * 
 * Esta versión NO contiene pragmas OpenMP.
 * 
 * @param img      Imagen de destino (debe estar previamente asignada).
 * @param x_min    Límite izquierdo del plano complejo.
 * @param x_max    Límite derecho del plano complejo.
 * @param y_min    Límite inferior del plano complejo.
 * @param y_max    Límite superior del plano complejo.
 * @param max_iter Número máximo de iteraciones.
 */
void generate_mandelbrot(Image *img, double x_min, double x_max,
                         double y_min, double y_max, int max_iter);

/**
 * generate_mandelbrot_parallel - Genera el fractal de Mandelbrot (versión paralela).
 * 
 * Idéntica a generate_mandelbrot pero con paralelización OpenMP en el
 * bucle externo (filas). Usa schedule(runtime) para que el planificador
 * se pueda configurar en tiempo de ejecución.
 * 
 * @param img      Imagen de destino (debe estar previamente asignada).
 * @param x_min    Límite izquierdo del plano complejo.
 * @param x_max    Límite derecho del plano complejo.
 * @param y_min    Límite inferior del plano complejo.
 * @param y_max    Límite superior del plano complejo.
 * @param max_iter Número máximo de iteraciones.
 */
void generate_mandelbrot_parallel(Image *img, double x_min, double x_max,
                                  double y_min, double y_max, int max_iter);

#endif /* MANDELBROT_H */
