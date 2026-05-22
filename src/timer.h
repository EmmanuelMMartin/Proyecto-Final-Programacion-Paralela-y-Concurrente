/**
 * ============================================================================
 * timer.h - Utilidad de medición de tiempo portable
 * ============================================================================
 * 
 * Proporciona funciones inline para medir el tiempo de ejecución.
 * 
 * Estrategia de temporización:
 *   - Si OpenMP está disponible (_OPENMP definido): usa omp_get_wtime()
 *     que mide tiempo de pared (wall-clock time) con alta resolución.
 *   - Si no: usa clock()/CLOCKS_PER_SEC como respaldo, que mide
 *     tiempo de CPU (no tiempo de pared).
 * 
 * Uso típico:
 *   double t_inicio = timer_start();
 *   // ... código a medir ...
 *   double t_fin = timer_stop();
 *   printf("Tiempo: %.4f segundos\n", timer_elapsed(t_inicio, t_fin));
 * 
 * Autor: Proyecto Programación Paralela
 * ============================================================================
 */

#ifndef TIMER_H
#define TIMER_H

#ifdef _OPENMP
    #include <omp.h>
#else
    #include <time.h>
#endif

/**
 * timer_start - Captura el instante de inicio de la medición.
 * 
 * @return Marca de tiempo en segundos (double).
 */
static inline double timer_start(void) {
    #ifdef _OPENMP
        /* omp_get_wtime() devuelve tiempo de pared en segundos */
        return omp_get_wtime();
    #else
        /* clock() devuelve ciclos de CPU; dividimos por CLOCKS_PER_SEC */
        return (double)clock() / CLOCKS_PER_SEC;
    #endif
}

/**
 * timer_stop - Captura el instante de finalización de la medición.
 * 
 * @return Marca de tiempo en segundos (double).
 */
static inline double timer_stop(void) {
    #ifdef _OPENMP
        return omp_get_wtime();
    #else
        return (double)clock() / CLOCKS_PER_SEC;
    #endif
}

/**
 * timer_elapsed - Calcula el tiempo transcurrido entre dos marcas.
 * 
 * @param start Marca de inicio (obtenida con timer_start).
 * @param stop  Marca de fin (obtenida con timer_stop).
 * @return Tiempo transcurrido en segundos.
 */
static inline double timer_elapsed(double start, double stop) {
    return stop - start;
}

/**
 * timer_print - Imprime el tiempo transcurrido con un mensaje descriptivo.
 * 
 * @param label Etiqueta descriptiva de la operación medida.
 * @param elapsed Tiempo en segundos.
 */
static inline void timer_print(const char *label, double elapsed) {
    printf("  [TIEMPO] %-40s : %10.4f segundos\n", label, elapsed);
}

/**
 * timer_print_comparison - Imprime comparación entre tiempos secuencial y paralelo.
 * 
 * @param label    Etiqueta descriptiva.
 * @param seq_time Tiempo de la versión secuencial.
 * @param par_time Tiempo de la versión paralela.
 */
static inline void timer_print_comparison(const char *label, double seq_time, double par_time) {
    double speedup = (par_time > 0.0) ? seq_time / par_time : 0.0;
    double efficiency = 0.0;
    
    #ifdef _OPENMP
        int num_threads = omp_get_max_threads();
        efficiency = (num_threads > 0) ? speedup / num_threads * 100.0 : 0.0;
    #else
        efficiency = speedup * 100.0;
    #endif
    
    printf("  [COMP]   %-40s : Seq=%8.4fs | Par=%8.4fs | Speedup=%.2fx",
           label, seq_time, par_time, speedup);
    printf(" | Eficiencia=%.1f%%\n", efficiency);
}

#endif /* TIMER_H */
