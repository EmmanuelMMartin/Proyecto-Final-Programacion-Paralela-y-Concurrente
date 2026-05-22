/**
 * ============================================================================
 * common.h - Definiciones compartidas para el proyecto Mandelbrot + Convolución
 * ============================================================================
 * 
 * Contiene las estructuras de datos fundamentales, constantes por defecto
 * y declaraciones de funciones comunes utilizadas en todo el proyecto.
 * 
 * Resolución por defecto: 7680x4320 (8K UHD)
 * Iteraciones máximas Mandelbrot: 1000
 * Entradas de paleta de colores: 256
 * 
 * Autor: Proyecto Programación Paralela
 * ============================================================================
 */

#ifndef COMMON_H
#define COMMON_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

/* ==============================
 * Constantes por defecto
 * ============================== */

/** Ancho de imagen por defecto (8K UHD) */
#define DEFAULT_WIDTH  7680

/** Alto de imagen por defecto (8K UHD) */
#define DEFAULT_HEIGHT 4320

/** Número máximo de iteraciones para el cálculo de Mandelbrot */
#define MAX_ITER 1000

/** Número de entradas en la paleta de colores (niveles por canal) */
#define NUM_COLORS 256

/** Valor de PI para cálculos matemáticos */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ==============================
 * Límites por defecto del plano complejo para Mandelbrot
 * ============================== */
#define DEFAULT_X_MIN -2.5
#define DEFAULT_X_MAX  1.0
#define DEFAULT_Y_MIN -1.25
#define DEFAULT_Y_MAX  1.25

/* ==============================
 * Radio de desenfoque gaussiano por defecto
 * ============================== */
#define DEFAULT_BLUR_RADIUS 10

/* ==============================
 * Estructuras de datos
 * ============================== */

/**
 * Color - Representa un pixel en formato RGB (8 bits por canal).
 * 
 * Cada componente (r, g, b) almacena un valor entre 0 y 255.
 */
typedef struct {
    unsigned char r;  /**< Componente rojo   (0-255) */
    unsigned char g;  /**< Componente verde  (0-255) */
    unsigned char b;  /**< Componente azul   (0-255) */
} Color;

/**
 * Image - Representa una imagen en formato RGB intercalado, fila por fila.
 * 
 * El arreglo de datos está organizado como:
 *   data[y * width * 3 + x * 3 + 0] = R
 *   data[y * width * 3 + x * 3 + 1] = G
 *   data[y * width * 3 + x * 3 + 2] = B
 * 
 * donde (x, y) es la posición del pixel.
 */
typedef struct {
    int width;            /**< Ancho de la imagen en pixeles */
    int height;           /**< Alto de la imagen en pixeles  */
    unsigned char *data;  /**< Datos RGB intercalados, fila por fila */
} Image;

/* ==============================
 * Macros de utilidad
 * ============================== */

/** Acceso al componente rojo del pixel en (x, y) */
#define PIXEL_R(img, x, y) ((img)->data[((y) * (img)->width + (x)) * 3 + 0])

/** Acceso al componente verde del pixel en (x, y) */
#define PIXEL_G(img, x, y) ((img)->data[((y) * (img)->width + (x)) * 3 + 1])

/** Acceso al componente azul del pixel en (x, y) */
#define PIXEL_B(img, x, y) ((img)->data[((y) * (img)->width + (x)) * 3 + 2])

/** Índice base del pixel en (x, y) en el arreglo de datos */
#define PIXEL_INDEX(img, x, y) (((y) * (img)->width + (x)) * 3)

/** Función para fijar un valor entre un mínimo y un máximo */
static inline int clamp_int(int val, int min_val, int max_val) {
    if (val < min_val) return min_val;
    if (val > max_val) return max_val;
    return val;
}

/** Función para fijar un valor double entre 0.0 y 1.0 */
static inline double clamp_double(double val, double min_val, double max_val) {
    if (val < min_val) return min_val;
    if (val > max_val) return max_val;
    return val;
}

#endif /* COMMON_H */
