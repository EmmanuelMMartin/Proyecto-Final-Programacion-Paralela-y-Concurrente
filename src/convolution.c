/**
 * ============================================================================
 * convolution.c - Implementación de convolución gaussiana para imágenes
 * ============================================================================
 * 
 * Implementa la creación de kernels gaussianos y tres versiones de
 * la convolución:
 *   1. Secuencial: sin OpenMP
 *   2. Paralela: con #pragma omp parallel for schedule(runtime)
 *   3. SIMD: con #pragma omp parallel for + #pragma omp simd
 * 
 * El kernel gaussiano 2D se genera como el producto de dos gaussianas 1D:
 *   G(x,y) = (1/(2*pi*sigma²)) * exp(-(x² + y²) / (2*sigma²))
 * 
 * Manejo de bordes: fijación (clamping) de índices al rango válido.
 * Esto replica los pixeles del borde en lugar de usar relleno con ceros.
 * 
 * Autor: Proyecto Programación Paralela
 * ============================================================================
 */

#include "convolution.h"

#ifdef _OPENMP
    #include <omp.h>
#endif

/* ============================================================================
 * create_gaussian_kernel - Crea un kernel gaussiano 2D normalizado.
 * 
 * El kernel tiene dimensión (2*radius + 1) × (2*radius + 1).
 * Si sigma <= 0, se usa sigma = radius / 3.0 (regla empírica que
 * asegura que el 99.7% de la distribución cabe en el kernel).
 * 
 * Los coeficientes se normalizan para que sumen exactamente 1.0,
 * lo que preserva el brillo de la imagen al aplicar la convolución.
 * ============================================================================ */
Kernel* create_gaussian_kernel(int radius, double sigma) {
    /* Validar radio */
    if (radius <= 0) {
        fprintf(stderr, "[ERROR] Radio de kernel inválido: %d\n", radius);
        return NULL;
    }

    /* Si sigma no fue especificado, usar la regla empírica */
    if (sigma <= 0.0) {
        sigma = (double)radius / 3.0;
    }

    /* Calcular el tamaño del kernel */
    int size = 2 * radius + 1;

    /* Asignar estructura del kernel */
    Kernel *k = (Kernel*)malloc(sizeof(Kernel));
    if (!k) {
        fprintf(stderr, "[ERROR] No se pudo asignar memoria para el kernel\n");
        return NULL;
    }

    k->size = size;
    k->data = (double*)malloc((size_t)size * (size_t)size * sizeof(double));
    if (!k->data) {
        fprintf(stderr, "[ERROR] No se pudo asignar memoria para datos del kernel (%dx%d)\n",
                size, size);
        free(k);
        return NULL;
    }

    /* 
     * Calcular coeficientes gaussianos:
     * G(x,y) = exp(-(x² + y²) / (2 * sigma²))
     * 
     * No incluimos el factor de normalización 1/(2*pi*sigma²)
     * porque normalizamos explícitamente después.
     */
    double sum = 0.0;
    double two_sigma2 = 2.0 * sigma * sigma;
    int ky, kx;

    for (ky = -radius; ky <= radius; ky++) {
        for (kx = -radius; kx <= radius; kx++) {
            double value = exp(-(double)(kx * kx + ky * ky) / two_sigma2);
            k->data[(ky + radius) * size + (kx + radius)] = value;
            sum += value;
        }
    }

    /* 
     * Normalizar: dividir cada coeficiente por la suma total
     * para que la suma de todos los coeficientes sea 1.0
     */
    if (sum > 0.0) {
        int total = size * size;
        int i;
        for (i = 0; i < total; i++) {
            k->data[i] /= sum;
        }
    }

    return k;
}

/* ============================================================================
 * free_kernel - Libera la memoria de un kernel de convolución.
 * ============================================================================ */
void free_kernel(Kernel *k) {
    if (k) {
        if (k->data) {
            free(k->data);
            k->data = NULL;
        }
        free(k);
    }
}

/* ============================================================================
 * apply_convolution - Versión SECUENCIAL de la convolución.
 * 
 * Para cada pixel de la imagen de salida:
 *   1. Centra el kernel sobre el pixel correspondiente de la entrada
 *   2. Multiplica cada coeficiente del kernel por el pixel vecino
 *   3. Suma los productos para obtener el nuevo valor del pixel
 * 
 * Manejo de bordes: se usa clamp (fijación) de coordenadas al rango
 * [0, width-1] × [0, height-1]. Esto equivale a replicar los pixeles
 * del borde, lo que produce resultados más naturales que rellenar con ceros.
 * 
 * Complejidad: O(width * height * kernel_size²)
 * ============================================================================ */
void apply_convolution(const Image *input, Image *output, const Kernel *kernel) {
    if (!input || !input->data || !output || !output->data || !kernel || !kernel->data) {
        fprintf(stderr, "[ERROR] Parámetros inválidos en apply_convolution\n");
        return;
    }

    int width  = input->width;
    int height = input->height;
    int ksize  = kernel->size;
    int radius = ksize / 2;
    int y, x, ky, kx;

    /* Recorrer cada pixel de la imagen */
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            /* Acumuladores para los tres canales */
            double sum_r = 0.0;
            double sum_g = 0.0;
            double sum_b = 0.0;

            /* Recorrer el kernel */
            for (ky = -radius; ky <= radius; ky++) {
                for (kx = -radius; kx <= radius; kx++) {
                    /* Coordenadas del pixel vecino con clamping */
                    int ny = clamp_int(y + ky, 0, height - 1);
                    int nx = clamp_int(x + kx, 0, width - 1);

                    /* Coeficiente del kernel */
                    double kval = kernel->data[(ky + radius) * ksize + (kx + radius)];

                    /* Índice del pixel vecino */
                    int nidx = PIXEL_INDEX(input, nx, ny);

                    /* Acumular producto */
                    sum_r += kval * (double)input->data[nidx + 0];
                    sum_g += kval * (double)input->data[nidx + 1];
                    sum_b += kval * (double)input->data[nidx + 2];
                }
            }

            /* Escribir resultado con fijación a [0, 255] */
            int oidx = PIXEL_INDEX(output, x, y);
            output->data[oidx + 0] = (unsigned char)clamp_int((int)(sum_r + 0.5), 0, 255);
            output->data[oidx + 1] = (unsigned char)clamp_int((int)(sum_g + 0.5), 0, 255);
            output->data[oidx + 2] = (unsigned char)clamp_int((int)(sum_b + 0.5), 0, 255);
        }
    }
}

/* ============================================================================
 * apply_convolution_parallel - Versión PARALELA de la convolución.
 * 
 * Idéntica a la versión secuencial pero con el bucle externo (filas)
 * paralelizado con OpenMP. Cada hilo procesa un subconjunto de filas
 * de forma independiente (no hay dependencias entre filas).
 * 
 * schedule(runtime) permite configurar el planificador dinámicamente.
 * ============================================================================ */
void apply_convolution_parallel(const Image *input, Image *output, const Kernel *kernel) {
    if (!input || !input->data || !output || !output->data || !kernel || !kernel->data) {
        fprintf(stderr, "[ERROR] Parámetros inválidos en apply_convolution_parallel\n");
        return;
    }

    int width  = input->width;
    int height = input->height;
    int ksize  = kernel->size;
    int radius = ksize / 2;
    int y, x, ky, kx;

    #ifdef _OPENMP
    #pragma omp parallel for schedule(runtime) private(x, ky, kx)
    #endif
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            double sum_r = 0.0;
            double sum_g = 0.0;
            double sum_b = 0.0;

            for (ky = -radius; ky <= radius; ky++) {
                for (kx = -radius; kx <= radius; kx++) {
                    int ny = clamp_int(y + ky, 0, height - 1);
                    int nx = clamp_int(x + kx, 0, width - 1);
                    double kval = kernel->data[(ky + radius) * ksize + (kx + radius)];
                    int nidx = PIXEL_INDEX(input, nx, ny);

                    sum_r += kval * (double)input->data[nidx + 0];
                    sum_g += kval * (double)input->data[nidx + 1];
                    sum_b += kval * (double)input->data[nidx + 2];
                }
            }

            int oidx = PIXEL_INDEX(output, x, y);
            output->data[oidx + 0] = (unsigned char)clamp_int((int)(sum_r + 0.5), 0, 255);
            output->data[oidx + 1] = (unsigned char)clamp_int((int)(sum_g + 0.5), 0, 255);
            output->data[oidx + 2] = (unsigned char)clamp_int((int)(sum_b + 0.5), 0, 255);
        }
    }
}

/* ============================================================================
 * apply_convolution_simd - Versión con paralelismo OpenMP + vectorización SIMD.
 * 
 * Combina paralelismo a nivel de hilos (parallel for) con vectorización
 * a nivel de instrucciones (omp simd) en los bucles internos del kernel.
 * 
 * La directiva #pragma omp simd indica al compilador que vectorice el
 * bucle interno, usando instrucciones SIMD (SSE, AVX, etc.) cuando
 * estén disponibles en el hardware.
 * 
 * Para facilitar la vectorización, se pre-calculan los índices de fila
 * del kernel y se procesan los canales con acumuladores separados.
 * ============================================================================ */
void apply_convolution_simd(const Image *input, Image *output, const Kernel *kernel) {
    if (!input || !input->data || !output || !output->data || !kernel || !kernel->data) {
        fprintf(stderr, "[ERROR] Parámetros inválidos en apply_convolution_simd\n");
        return;
    }

    int width  = input->width;
    int height = input->height;
    int ksize  = kernel->size;
    int radius = ksize / 2;
    int y;

    #ifdef _OPENMP
    #pragma omp parallel for schedule(runtime)
    #endif
    for (y = 0; y < height; y++) {
        int x, ky, kx;

        for (x = 0; x < width; x++) {
            double sum_r = 0.0;
            double sum_g = 0.0;
            double sum_b = 0.0;

            for (ky = -radius; ky <= radius; ky++) {
                /* Pre-calcular la fila fijada del vecino */
                int ny = clamp_int(y + ky, 0, height - 1);
                int row_offset = ny * width * 3;
                int krow_offset = (ky + radius) * ksize;

                /* 
                 * Bucle interno vectorizado con SIMD:
                 * Procesa las columnas del kernel con instrucciones vectoriales
                 */
                #ifdef _OPENMP
                #pragma omp simd reduction(+:sum_r, sum_g, sum_b)
                #endif
                for (kx = -radius; kx <= radius; kx++) {
                    int nx = clamp_int(x + kx, 0, width - 1);
                    double kval = kernel->data[krow_offset + (kx + radius)];
                    int nidx = row_offset + nx * 3;

                    sum_r += kval * (double)input->data[nidx + 0];
                    sum_g += kval * (double)input->data[nidx + 1];
                    sum_b += kval * (double)input->data[nidx + 2];
                }
            }

            int oidx = PIXEL_INDEX(output, x, y);
            output->data[oidx + 0] = (unsigned char)clamp_int((int)(sum_r + 0.5), 0, 255);
            output->data[oidx + 1] = (unsigned char)clamp_int((int)(sum_g + 0.5), 0, 255);
            output->data[oidx + 2] = (unsigned char)clamp_int((int)(sum_b + 0.5), 0, 255);
        }
    }
}
