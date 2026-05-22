/**
 * ============================================================================
 * convolution.h - Convolución Gaussiana para filtrado de imágenes
 * ============================================================================
 * 
 * Proporciona funciones para crear kernels gaussianos y aplicar
 * convolución a imágenes en formato RGB.
 * 
 * Versiones de convolución:
 *   - Secuencial: sin pragmas OpenMP
 *   - Paralela: con #pragma omp parallel for schedule(runtime)
 *   - SIMD: con #pragma omp parallel for + #pragma omp simd
 * 
 * Manejo de bordes: fijación (clamping) de índices al rango válido.
 * 
 * Autor: Proyecto Programación Paralela
 * ============================================================================
 */

#ifndef CONVOLUTION_H
#define CONVOLUTION_H

#include "common.h"

/**
 * Kernel - Estructura para representar un kernel de convolución 2D.
 * 
 * El kernel es cuadrado de dimensión (size x size).
 * Los datos se almacenan en orden fila por fila.
 */
typedef struct {
    int size;       /**< Dimensión del kernel (size x size) */
    double *data;   /**< Coeficientes del kernel, tamaño size*size */
} Kernel;

/**
 * create_gaussian_kernel - Crea un kernel gaussiano 2D normalizado.
 * 
 * Genera un kernel de tamaño (2*radius + 1) x (2*radius + 1)
 * con distribución gaussiana. Los coeficientes se normalizan para
 * que sumen 1.0.
 * 
 * @param radius Radio del kernel (el tamaño será 2*radius + 1).
 * @param sigma  Desviación estándar de la gaussiana. Si sigma <= 0,
 *               se usa sigma = radius / 3.0 por defecto.
 * @return Puntero al kernel creado, o NULL si falla.
 */
Kernel* create_gaussian_kernel(int radius, double sigma);

/**
 * free_kernel - Libera la memoria de un kernel.
 * 
 * @param k Puntero al kernel a liberar. Es seguro pasar NULL.
 */
void free_kernel(Kernel *k);

/**
 * apply_convolution - Aplica convolución a una imagen (versión secuencial).
 * 
 * Los bordes se manejan con fijación de índices (clamping).
 * Esta versión NO contiene pragmas OpenMP.
 * 
 * @param input  Imagen de entrada (no se modifica).
 * @param output Imagen de salida (debe estar previamente asignada).
 * @param kernel Kernel de convolución a aplicar.
 */
void apply_convolution(const Image *input, Image *output, const Kernel *kernel);

/**
 * apply_convolution_parallel - Aplica convolución con paralelización OpenMP.
 * 
 * Usa #pragma omp parallel for schedule(runtime) en el bucle externo.
 * 
 * @param input  Imagen de entrada (no se modifica).
 * @param output Imagen de salida (debe estar previamente asignada).
 * @param kernel Kernel de convolución a aplicar.
 */
void apply_convolution_parallel(const Image *input, Image *output, const Kernel *kernel);

/**
 * apply_convolution_simd - Aplica convolución con OpenMP paralelo + SIMD.
 * 
 * Usa #pragma omp parallel for en el bucle externo y
 * #pragma omp simd en los bucles internos para vectorización.
 * 
 * @param input  Imagen de entrada (no se modifica).
 * @param output Imagen de salida (debe estar previamente asignada).
 * @param kernel Kernel de convolución a aplicar.
 */
void apply_convolution_simd(const Image *input, Image *output, const Kernel *kernel);

#endif /* CONVOLUTION_H */
