/**
 * ============================================================================
 * histogram.c - Implementación de histogramas de color con múltiples estrategias
 * ============================================================================
 * 
 * Implementa 5 estrategias diferentes para calcular histogramas RGB,
 * cada una demostrando diferentes técnicas de paralelización OpenMP:
 * 
 *   1. Secuencial: Referencia sin paralelización.
 *   2. Atomic: Operaciones atómicas por cada actualización.
 *   3. Critical: Sección crítica que serializa actualizaciones.
 *   4. Reduction: Arreglos locales por hilo + reducción manual (óptima).
 *   5. False Sharing: Deliberadamente mala para demostrar el problema.
 * 
 * El propósito educativo es mostrar cómo diferentes estrategias de
 * sincronización afectan el rendimiento en la práctica.
 * 
 * Autor: Proyecto Programación Paralela
 * ============================================================================
 */

#include "histogram.h"

#ifdef _OPENMP
    #include <omp.h>
#endif

/* ============================================================================
 * Función auxiliar: inicializar histogramas a cero.
 * ============================================================================ */
static void clear_histogram(long long hist_r[HIST_SIZE],
                            long long hist_g[HIST_SIZE],
                            long long hist_b[HIST_SIZE]) {
    memset(hist_r, 0, HIST_SIZE * sizeof(long long));
    memset(hist_g, 0, HIST_SIZE * sizeof(long long));
    memset(hist_b, 0, HIST_SIZE * sizeof(long long));
}

/* ============================================================================
 * compute_histogram_sequential - Versión SECUENCIAL.
 * 
 * Recorre todos los pixeles de la imagen secuencialmente y cuenta
 * las ocurrencias de cada valor de intensidad (0-255) por canal.
 * 
 * Esta versión sirve como referencia para verificar la corrección
 * de las versiones paralelas y para medir el speedup.
 * 
 * Complejidad: O(width * height)
 * ============================================================================ */
void compute_histogram_sequential(const Image *img,
                                  long long hist_r[HIST_SIZE],
                                  long long hist_g[HIST_SIZE],
                                  long long hist_b[HIST_SIZE]) {
    if (!img || !img->data) {
        fprintf(stderr, "[ERROR] Imagen inválida en compute_histogram_sequential\n");
        return;
    }

    /* Inicializar histogramas a cero */
    clear_histogram(hist_r, hist_g, hist_b);

    size_t total_pixels = (size_t)img->width * (size_t)img->height;
    size_t i;

    /* Recorrer cada pixel y acumular en los histogramas */
    for (i = 0; i < total_pixels; i++) {
        unsigned char r = img->data[i * 3 + 0];
        unsigned char g = img->data[i * 3 + 1];
        unsigned char b = img->data[i * 3 + 2];

        hist_r[r]++;
        hist_g[g]++;
        hist_b[b]++;
    }
}

/* ============================================================================
 * compute_histogram_atomic - Versión con #pragma omp atomic.
 * 
 * Cada hilo actualiza directamente los histogramas compartidos usando
 * operaciones atómicas. Esto garantiza la corrección sin secciones
 * críticas completas.
 * 
 * Rendimiento: moderado. Las operaciones atómicas son más eficientes
 * que las secciones críticas, pero hay contención cuando múltiples
 * hilos intentan actualizar la misma posición del histograma.
 * 
 * Con imágenes grandes y 256 posiciones, la probabilidad de colisión
 * es relativamente baja, por lo que el rendimiento es aceptable.
 * ============================================================================ */
void compute_histogram_atomic(const Image *img,
                              long long hist_r[HIST_SIZE],
                              long long hist_g[HIST_SIZE],
                              long long hist_b[HIST_SIZE]) {
    if (!img || !img->data) {
        fprintf(stderr, "[ERROR] Imagen inválida en compute_histogram_atomic\n");
        return;
    }

    clear_histogram(hist_r, hist_g, hist_b);

    long long total_pixels = (long long)img->width * (long long)img->height;
    long long i;

    #ifdef _OPENMP
    #pragma omp parallel for schedule(static)
    #endif
    for (i = 0; i < total_pixels; i++) {
        unsigned char r = img->data[i * 3 + 0];
        unsigned char g = img->data[i * 3 + 1];
        unsigned char b = img->data[i * 3 + 2];

        /* 
         * Operaciones atómicas: cada incremento es una operación
         * read-modify-write atómica a nivel de hardware.
         */
        #ifdef _OPENMP
        #pragma omp atomic
        #endif
        hist_r[r]++;
        
        #ifdef _OPENMP
        #pragma omp atomic
        #endif
        hist_g[g]++;
        
        #ifdef _OPENMP
        #pragma omp atomic
        #endif
        hist_b[b]++;
    }
}

/* ============================================================================
 * compute_histogram_critical - Versión con #pragma omp critical.
 * 
 * Usa una sección crítica para proteger TODAS las actualizaciones
 * del histograma. Esto es MUY ineficiente porque:
 *   - Solo un hilo puede ejecutar la sección crítica a la vez
 *   - Los demás hilos quedan bloqueados esperando
 *   - El overhead de adquirir/liberar el mutex es significativo
 * 
 * Se incluye para demostrar que critical NO es apropiado para
 * operaciones de granularidad fina como incrementos de contadores.
 * 
 * Rendimiento: MALO. Frecuentemente peor que la versión secuencial
 * debido al overhead de sincronización.
 * ============================================================================ */
void compute_histogram_critical(const Image *img,
                                long long hist_r[HIST_SIZE],
                                long long hist_g[HIST_SIZE],
                                long long hist_b[HIST_SIZE]) {
    if (!img || !img->data) {
        fprintf(stderr, "[ERROR] Imagen inválida en compute_histogram_critical\n");
        return;
    }

    clear_histogram(hist_r, hist_g, hist_b);

    long long total_pixels = (long long)img->width * (long long)img->height;
    long long i;

    #ifdef _OPENMP
    #pragma omp parallel for schedule(static)
    #endif
    for (i = 0; i < total_pixels; i++) {
        unsigned char r = img->data[i * 3 + 0];
        unsigned char g = img->data[i * 3 + 1];
        unsigned char b = img->data[i * 3 + 2];

        /* 
         * Sección crítica: TODOS los incrementos protegidos.
         * Esto serializa efectivamente la operación.
         * ¡Solo un hilo puede incrementar a la vez!
         */
        #ifdef _OPENMP
        #pragma omp critical
        #endif
        {
            hist_r[r]++;
            hist_g[g]++;
            hist_b[b]++;
        }
    }
}

/* ============================================================================
 * compute_histogram_reduction - Versión con reducción manual.
 * 
 * Esta es la ESTRATEGIA ÓPTIMA para histogramas paralelos:
 *   1. Cada hilo mantiene sus propios arreglos locales (privados)
 *   2. Cada hilo llena su histograma local sin sincronización
 *   3. Al final, se combinan los histogramas locales en el global
 * 
 * Ventajas:
 *   - Sin contención durante el cálculo (cada hilo usa su propia memoria)
 *   - Sin false sharing (arreglos locales están en cache del propio hilo)
 *   - La fase de reducción es O(HIST_SIZE * num_threads), muy rápida
 * 
 * Desventaja: Usa más memoria (HIST_SIZE * 3 * num_threads * sizeof(long long))
 * Pero para HIST_SIZE=256, esto es solo ~6KB por hilo, insignificante.
 * ============================================================================ */
void compute_histogram_reduction(const Image *img,
                                 long long hist_r[HIST_SIZE],
                                 long long hist_g[HIST_SIZE],
                                 long long hist_b[HIST_SIZE]) {
    if (!img || !img->data) {
        fprintf(stderr, "[ERROR] Imagen inválida en compute_histogram_reduction\n");
        return;
    }

    clear_histogram(hist_r, hist_g, hist_b);

    long long total_pixels = (long long)img->width * (long long)img->height;

    #ifdef _OPENMP
    #pragma omp parallel
    {
        /* Arreglos locales por hilo: completamente privados */
        long long local_r[HIST_SIZE];
        long long local_g[HIST_SIZE];
        long long local_b[HIST_SIZE];
        long long i;

        /* Inicializar arreglos locales a cero */
        memset(local_r, 0, HIST_SIZE * sizeof(long long));
        memset(local_g, 0, HIST_SIZE * sizeof(long long));
        memset(local_b, 0, HIST_SIZE * sizeof(long long));

        /* Fase 1: Cada hilo llena su histograma local sin sincronización */
        #pragma omp for schedule(static) nowait
        for (i = 0; i < total_pixels; i++) {
            unsigned char r = img->data[i * 3 + 0];
            unsigned char g = img->data[i * 3 + 1];
            unsigned char b = img->data[i * 3 + 2];

            local_r[r]++;
            local_g[g]++;
            local_b[b]++;
        }

        /* 
         * Fase 2: Reducción manual - combinar histogramas locales
         * Usamos atomic aquí, pero solo se ejecuta 256 veces por hilo
         * (en lugar de millones de veces como en la versión atomic pura)
         */
        {
            int j;
            for (j = 0; j < HIST_SIZE; j++) {
                #pragma omp atomic
                hist_r[j] += local_r[j];
                #pragma omp atomic
                hist_g[j] += local_g[j];
                #pragma omp atomic
                hist_b[j] += local_b[j];
            }
        }
    }
    #else
    /* Versión sin OpenMP: equivale a secuencial */
    {
        long long i;
        for (i = 0; i < total_pixels; i++) {
            unsigned char r = img->data[i * 3 + 0];
            unsigned char g = img->data[i * 3 + 1];
            unsigned char b = img->data[i * 3 + 2];
            hist_r[r]++;
            hist_g[g]++;
            hist_b[b]++;
        }
    }
    #endif
}

/* ============================================================================
 * compute_histogram_false_sharing - Versión con COMPARTICIÓN FALSA deliberada.
 * 
 * ¡VERSIÓN INTENCIONALMENTE INEFICIENTE!
 * 
 * Demuestra el problema de "false sharing" (compartición falsa) de caché:
 *   - Se crea un arreglo compartido donde cada hilo escribe en posiciones
 *     adyacentes (separadas solo por sizeof(long long) = 8 bytes)
 *   - Las líneas de caché típicas tienen 64 bytes, por lo que 8 contadores
 *     de 8 bytes caben en una sola línea de caché
 *   - Cuando un hilo modifica su contador, invalida la línea de caché
 *     completa para TODOS los demás hilos
 *   - Esto causa constantes transferencias de caché entre núcleos
 *     (cache line ping-pong), destruyendo el rendimiento
 * 
 * Rendimiento esperado: PEOR que la versión secuencial en la mayoría
 * de arquitecturas multi-core modernas.
 * ============================================================================ */
void compute_histogram_false_sharing(const Image *img,
                                     long long hist_r[HIST_SIZE],
                                     long long hist_g[HIST_SIZE],
                                     long long hist_b[HIST_SIZE]) {
    if (!img || !img->data) {
        fprintf(stderr, "[ERROR] Imagen inválida en compute_histogram_false_sharing\n");
        return;
    }

    clear_histogram(hist_r, hist_g, hist_b);

    #ifdef _OPENMP
    int num_threads = omp_get_max_threads();
    
    /* 
     * Arreglo compartido con contadores ADYACENTES por hilo.
     * Estructura: [hilo0_r][hilo1_r][hilo2_r]...[hilo0_g][hilo1_g]...
     * 
     * Los contadores de hilos adyacentes comparten líneas de caché,
     * causando invalidaciones constantes (false sharing).
     * 
     * Cada hilo tiene: hist_all[canal * HIST_SIZE * num_threads + valor * num_threads + tid]
     */
    size_t arr_size = (size_t)3 * HIST_SIZE * (size_t)num_threads;
    long long *hist_all = (long long*)calloc(arr_size, sizeof(long long));
    if (!hist_all) {
        fprintf(stderr, "[ERROR] No se pudo asignar memoria para histograma false_sharing\n");
        return;
    }

    long long total_pixels = (long long)img->width * (long long)img->height;

    #pragma omp parallel
    {
        int tid = omp_get_thread_num();
        long long i;

        /* 
         * Cada hilo escribe en posiciones adyacentes:
         *   hist_all[valor * num_threads + tid] para canal R
         *   hist_all[HIST_SIZE * num_threads + valor * num_threads + tid] para canal G
         *   hist_all[2 * HIST_SIZE * num_threads + valor * num_threads + tid] para canal B
         * 
         * Notar que hist_all[val * num_threads + 0] y hist_all[val * num_threads + 1]
         * están a solo 8 bytes de distancia = FALSE SHARING GARANTIZADO
         */
        #pragma omp for schedule(static)
        for (i = 0; i < total_pixels; i++) {
            unsigned char r = img->data[i * 3 + 0];
            unsigned char g = img->data[i * 3 + 1];
            unsigned char b = img->data[i * 3 + 2];

            /* Escrituras adyacentes entre hilos: ¡false sharing! */
            hist_all[(size_t)r * num_threads + tid]++;
            hist_all[(size_t)HIST_SIZE * num_threads + (size_t)g * num_threads + tid]++;
            hist_all[(size_t)2 * HIST_SIZE * num_threads + (size_t)b * num_threads + tid]++;
        }
    }

    /* Reducción: combinar los contadores de todos los hilos */
    {
        int j, t;
        for (j = 0; j < HIST_SIZE; j++) {
            for (t = 0; t < num_threads; t++) {
                hist_r[j] += hist_all[(size_t)j * num_threads + t];
                hist_g[j] += hist_all[(size_t)HIST_SIZE * num_threads + (size_t)j * num_threads + t];
                hist_b[j] += hist_all[(size_t)2 * HIST_SIZE * num_threads + (size_t)j * num_threads + t];
            }
        }
    }

    free(hist_all);

    #else
    /* Sin OpenMP: equivale a secuencial */
    {
        size_t total_pixels = (size_t)img->width * (size_t)img->height;
        size_t i;
        for (i = 0; i < total_pixels; i++) {
            unsigned char r = img->data[i * 3 + 0];
            unsigned char g = img->data[i * 3 + 1];
            unsigned char b = img->data[i * 3 + 2];
            hist_r[r]++;
            hist_g[g]++;
            hist_b[b]++;
        }
    }
    #endif
}

/* ============================================================================
 * print_histogram_summary - Imprime estadísticas del histograma.
 * 
 * Muestra para cada canal (R, G, B):
 *   - Total de pixeles contados
 *   - Valor medio (promedio ponderado)
 *   - Valor más frecuente (moda)
 *   - Desviación estándar
 *   - Distribución visual simplificada (barras de texto)
 * ============================================================================ */
void print_histogram_summary(const long long hist_r[HIST_SIZE],
                             const long long hist_g[HIST_SIZE],
                             const long long hist_b[HIST_SIZE]) {
    int i;
    
    printf("\n  ╔══════════════════════════════════════════════════════╗\n");
    printf("  ║           RESUMEN DEL HISTOGRAMA DE COLOR           ║\n");
    printf("  ╠══════════════════════════════════════════════════════╣\n");

    /* Arreglo de canales para procesamiento uniforme */
    const long long *histograms[3] = { hist_r, hist_g, hist_b };
    const char *channel_names[3] = { "Rojo  (R)", "Verde (G)", "Azul  (B)" };
    const char *bar_chars[3] = { "#", "+", "=" };

    int ch;
    for (ch = 0; ch < 3; ch++) {
        const long long *hist = histograms[ch];

        /* Calcular estadísticas */
        long long total = 0;
        double weighted_sum = 0.0;
        long long max_count = 0;
        int mode = 0;

        for (i = 0; i < HIST_SIZE; i++) {
            total += hist[i];
            weighted_sum += (double)i * (double)hist[i];
            if (hist[i] > max_count) {
                max_count = hist[i];
                mode = i;
            }
        }

        double mean = (total > 0) ? weighted_sum / (double)total : 0.0;

        /* Desviación estándar */
        double var_sum = 0.0;
        for (i = 0; i < HIST_SIZE; i++) {
            double diff = (double)i - mean;
            var_sum += diff * diff * (double)hist[i];
        }
        double stddev = (total > 0) ? sqrt(var_sum / (double)total) : 0.0;

        printf("  ║  Canal: %-10s                                   ║\n", channel_names[ch]);
        printf("  ║    Total pixeles : %15lld                  ║\n", total);
        printf("  ║    Media         : %15.2f                  ║\n", mean);
        printf("  ║    Moda (valor)  : %15d (conteo: %lld)  \n", mode, max_count);
        printf("  ║    Desv. estándar: %15.2f                  ║\n", stddev);

        /* Distribución visual: 8 rangos de 32 valores cada uno */
        printf("  ║    Distribución:                                    ║\n");
        int range;
        for (range = 0; range < 8; range++) {
            int start = range * 32;
            int end = start + 31;
            long long range_count = 0;
            int j;
            for (j = start; j <= end; j++) {
                range_count += hist[j];
            }
            /* Normalizar a un ancho máximo de 30 caracteres */
            int bar_len = 0;
            if (total > 0 && max_count > 0) {
                /* Escalar respecto al rango más poblado */
                long long max_range = 0;
                int r2;
                for (r2 = 0; r2 < 8; r2++) {
                    long long rc = 0;
                    int j2;
                    for (j2 = r2 * 32; j2 < r2 * 32 + 32; j2++) {
                        rc += hist[j2];
                    }
                    if (rc > max_range) max_range = rc;
                }
                bar_len = (max_range > 0) ? (int)(30.0 * (double)range_count / (double)max_range) : 0;
            }
            printf("  ║    %3d-%3d: ", start, end);
            int k;
            for (k = 0; k < bar_len; k++) {
                printf("%s", bar_chars[ch]);
            }
            for (k = bar_len; k < 30; k++) {
                printf(" ");
            }
            printf(" ║\n");
        }

        if (ch < 2) {
            printf("  ╠══════════════════════════════════════════════════════╣\n");
        }
    }

    printf("  ╚══════════════════════════════════════════════════════╝\n\n");
}
