/**
 * ============================================================================
 * mandelbrot.c - Implementación de la generación del fractal de Mandelbrot
 * ============================================================================
 * 
 * Implementa la generación del conjunto de Mandelbrot con:
 *   - Coloración suave usando escape time continuo (log-based)
 *   - Conversión de paleta HSV a RGB con ciclo de matiz (hue cycling)
 *   - Versión secuencial (sin OpenMP)
 *   - Versión paralela (con #pragma omp parallel for)
 * 
 * Algoritmo de coloración suave:
 *   En lugar de usar directamente el número de iteraciones (que produce
 *   bandas de color), se calcula un valor continuo usando:
 *     smooth_iter = iter + 1 - log2(log2(|z|))
 *   
 *   Esto elimina las bandas y produce transiciones suaves entre colores.
 * 
 * Autor: Proyecto Programación Paralela
 * ============================================================================
 */

#include "mandelbrot.h"

#ifdef _OPENMP
    #include <omp.h>
#endif

/* ============================================================================
 * Funciones auxiliares internas (estáticas)
 * ============================================================================ */

/**
 * hsv_to_rgb - Convierte un color del espacio HSV al espacio RGB.
 * 
 * El espacio HSV (Hue, Saturation, Value) es más intuitivo para crear
 * paletas de colores cíclicas que el espacio RGB directo.
 * 
 * @param h Matiz (hue) en rango [0, 360).
 * @param s Saturación en rango [0, 1].
 * @param v Valor (brillo) en rango [0, 1].
 * @param r Puntero de salida para componente rojo (0-255).
 * @param g Puntero de salida para componente verde (0-255).
 * @param b Puntero de salida para componente azul (0-255).
 */
static void hsv_to_rgb(double h, double s, double v,
                       unsigned char *r, unsigned char *g, unsigned char *b) {
    /* Normalizar matiz al rango [0, 360) */
    while (h >= 360.0) h -= 360.0;
    while (h < 0.0) h += 360.0;

    double c = v * s;             /* Croma */
    double x = c * (1.0 - fabs(fmod(h / 60.0, 2.0) - 1.0));
    double m = v - c;             /* Ajuste de brillo */

    double r1, g1, b1;

    /* Seleccionar sector del hexágono de color */
    if (h < 60.0) {
        r1 = c; g1 = x; b1 = 0.0;
    } else if (h < 120.0) {
        r1 = x; g1 = c; b1 = 0.0;
    } else if (h < 180.0) {
        r1 = 0.0; g1 = c; b1 = x;
    } else if (h < 240.0) {
        r1 = 0.0; g1 = x; b1 = c;
    } else if (h < 300.0) {
        r1 = x; g1 = 0.0; b1 = c;
    } else {
        r1 = c; g1 = 0.0; b1 = x;
    }

    /* Convertir a rango 0-255 */
    *r = (unsigned char)((r1 + m) * 255.0 + 0.5);
    *g = (unsigned char)((g1 + m) * 255.0 + 0.5);
    *b = (unsigned char)((b1 + m) * 255.0 + 0.5);
}

/**
 * map_color - Asigna un color a un valor de iteración continuo.
 * 
 * Utiliza la paleta HSV cíclica con las siguientes características:
 *   - El matiz cicla suavemente para crear transiciones hermosas
 *   - La saturación varía ligeramente para dar profundidad
 *   - El brillo se ajusta para destacar detalles
 *   - Los puntos dentro del conjunto (max_iter alcanzado) son negros
 * 
 * @param smooth_iter Valor de iteración continuo (puede ser fraccionario).
 * @param max_iter    Número máximo de iteraciones.
 * @param r           Puntero de salida para componente rojo.
 * @param g           Puntero de salida para componente verde.
 * @param b           Puntero de salida para componente azul.
 */
static void map_color(double smooth_iter, int max_iter,
                      unsigned char *r, unsigned char *g, unsigned char *b) {
    /* Puntos dentro del conjunto: color negro */
    if (smooth_iter >= (double)max_iter) {
        *r = 0;
        *g = 0;
        *b = 0;
        return;
    }

    /* 
     * Paleta de colores usando HSV con múltiples ciclos:
     *   - El matiz gira 3 veces por el espectro completo
     *   - La saturación oscila suavemente entre 0.7 y 1.0
     *   - El brillo varía para crear efecto de profundidad
     */
    double t = smooth_iter / 50.0;  /* Factor de escala para la paleta */

    /* Matiz: ciclo continuo por el espectro con velocidad variable */
    double hue = fmod(360.0 * t, 360.0);

    /* Saturación: varía suavemente para dar riqueza al color */
    double saturation = 0.7 + 0.3 * sin(t * M_PI * 0.5);

    /* Valor (brillo): modulado para crear profundidad */
    double value = 0.6 + 0.4 * cos(t * M_PI * 0.3);
    if (value > 1.0) value = 1.0;
    if (value < 0.3) value = 0.3;

    hsv_to_rgb(hue, saturation, value, r, g, b);
}

/**
 * compute_mandelbrot_pixel - Calcula el color de un pixel de Mandelbrot.
 * 
 * Para cada pixel (px, py) de la imagen:
 *   1. Mapea las coordenadas del pixel al plano complejo (cx, cy)
 *   2. Itera z = z² + c hasta que |z| > 2 o se alcancen max_iter
 *   3. Calcula el valor de iteración suave (smooth coloring)
 *   4. Asigna un color usando la paleta HSV
 * 
 * @param px       Coordenada X del pixel.
 * @param py       Coordenada Y del pixel.
 * @param width    Ancho de la imagen.
 * @param height   Alto de la imagen.
 * @param x_min    Límite izquierdo del plano complejo.
 * @param x_max    Límite derecho del plano complejo.
 * @param y_min    Límite inferior del plano complejo.
 * @param y_max    Límite superior del plano complejo.
 * @param max_iter Máximo de iteraciones.
 * @param r        Puntero de salida para componente rojo.
 * @param g        Puntero de salida para componente verde.
 * @param b        Puntero de salida para componente azul.
 */
static void compute_mandelbrot_pixel(int px, int py,
                                     int width, int height,
                                     double x_min, double x_max,
                                     double y_min, double y_max,
                                     int max_iter,
                                     unsigned char *r, unsigned char *g, unsigned char *b) {
    /* Mapear coordenadas de pixel al plano complejo */
    double cx = x_min + (x_max - x_min) * (double)px / (double)(width - 1);
    double cy = y_min + (y_max - y_min) * (double)py / (double)(height - 1);

    /* Variables de la iteración z = z² + c */
    double zx = 0.0;   /* Parte real de z */
    double zy = 0.0;   /* Parte imaginaria de z */
    double zx2, zy2;   /* Cuadrados pre-calculados para optimización */
    int iter = 0;

    /* 
     * Iterar z = z² + c
     * 
     * Optimización: en lugar de calcular |z| = sqrt(zx² + zy²) > 2,
     * comparamos zx² + zy² > 4 (evitamos la raíz cuadrada).
     * 
     * Usamos radio de escape = 256 (en lugar de 2) para mejorar
     * la precisión del cálculo de coloración suave.
     */
    zx2 = zx * zx;
    zy2 = zy * zy;

    while (zx2 + zy2 <= 65536.0 && iter < max_iter) {
        /* z = z² + c: (zx + zy*i)² = zx² - zy² + 2*zx*zy*i */
        zy = 2.0 * zx * zy + cy;
        zx = zx2 - zy2 + cx;
        zx2 = zx * zx;
        zy2 = zy * zy;
        iter++;
    }

    /* Calcular coloración suave */
    if (iter < max_iter) {
        /* 
         * Coloración continua (smooth coloring):
         * smooth = iter + 1 - log2(log2(|z|))
         * 
         * donde |z|² = zx² + zy², por lo que log2(|z|) = 0.5 * log2(|z|²)
         * 
         * Esto da un valor continuo que elimina las bandas de color.
         */
        double log_zn = log(zx2 + zy2) / 2.0;       /* log(|z|) */
        double nu = log(log_zn / log(2.0)) / log(2.0); /* log2(log2(|z|)) */
        double smooth_iter = (double)iter + 1.0 - nu;
        
        /* Asegurar que el valor sea positivo */
        if (smooth_iter < 0.0) smooth_iter = 0.0;
        
        map_color(smooth_iter, max_iter, r, g, b);
    } else {
        /* Punto dentro del conjunto de Mandelbrot: negro */
        *r = 0;
        *g = 0;
        *b = 0;
    }
}

/* ============================================================================
 * generate_mandelbrot - Versión SECUENCIAL (sin OpenMP)
 * 
 * Recorre todos los pixeles de la imagen secuencialmente,
 * calculando el fractal de Mandelbrot para cada uno.
 * 
 * Complejidad: O(width * height * max_iter) en el peor caso.
 * ============================================================================ */
void generate_mandelbrot(Image *img, double x_min, double x_max,
                         double y_min, double y_max, int max_iter) {
    if (!img || !img->data) {
        fprintf(stderr, "[ERROR] Imagen inválida en generate_mandelbrot\n");
        return;
    }

    int width  = img->width;
    int height = img->height;
    int y, x;

    /* Recorrer cada fila (bucle externo en Y) */
    for (y = 0; y < height; y++) {
        /* Recorrer cada columna (bucle interno en X) */
        for (x = 0; x < width; x++) {
            unsigned char r, g, b;

            /* Calcular el color del pixel */
            compute_mandelbrot_pixel(x, y, width, height,
                                     x_min, x_max, y_min, y_max,
                                     max_iter, &r, &g, &b);

            /* Almacenar el color en la imagen */
            int idx = PIXEL_INDEX(img, x, y);
            img->data[idx + 0] = r;
            img->data[idx + 1] = g;
            img->data[idx + 2] = b;
        }
    }
}

/* ============================================================================
 * generate_mandelbrot_parallel - Versión PARALELA (con OpenMP)
 * 
 * Idéntica a la versión secuencial pero con paralelización en el
 * bucle externo (filas). Cada hilo procesa un subconjunto de filas.
 * 
 * Directivas OpenMP:
 *   - parallel for: distribuye las filas entre los hilos
 *   - schedule(runtime): permite configurar el planificador en ejecución
 *     mediante la variable OMP_SCHEDULE o omp_set_schedule()
 *   - collapse(1): no colapsa bucles (solo paraleliza el externo)
 *     Nota: se usa collapse(1) explícitamente para claridad
 * 
 * El bucle interno (columnas) se ejecuta secuencialmente dentro de
 * cada hilo, lo cual es eficiente porque cada fila tiene trabajo
 * variable (las filas del centro tienen más iteraciones).
 * ============================================================================ */
void generate_mandelbrot_parallel(Image *img, double x_min, double x_max,
                                  double y_min, double y_max, int max_iter) {
    if (!img || !img->data) {
        fprintf(stderr, "[ERROR] Imagen inválida en generate_mandelbrot_parallel\n");
        return;
    }

    int width  = img->width;
    int height = img->height;
    int y, x;

    /* 
     * Paralelización del bucle externo (filas):
     * - Cada hilo procesa filas completas
     * - schedule(runtime) permite elegir static/dynamic/guided en ejecución
     * - collapse(1) paraleliza solo el bucle de filas
     * - Las variables x, r, g, b son privadas por defecto (declaradas dentro)
     */
    #ifdef _OPENMP
    #pragma omp parallel for schedule(runtime) collapse(1) private(x)
    #endif
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            unsigned char r, g, b;

            compute_mandelbrot_pixel(x, y, width, height,
                                     x_min, x_max, y_min, y_max,
                                     max_iter, &r, &g, &b);

            int idx = PIXEL_INDEX(img, x, y);
            img->data[idx + 0] = r;
            img->data[idx + 1] = g;
            img->data[idx + 2] = b;
        }
    }
}
