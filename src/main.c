/**
 * ============================================================================
 * main.c - Programa principal: Fractal Mandelbrot + Convolución Gaussiana
 * ============================================================================
 * 
 * Programa completo que integra todas las funcionalidades del proyecto:
 *   1. Generación del fractal de Mandelbrot (secuencial y paralelo)
 *   2. Aplicación de filtro de convolución gaussiana (3 versiones)
 *   3. Cálculo de histogramas de color (5 estrategias)
 *   4. Medición y comparación de tiempos de ejecución
 * 
 * Soporta configuración completa mediante argumentos de línea de comandos.
 * 
 * Compilación:
 *   Sin OpenMP:  gcc -O2 -o fractal src/*.c -lm
 *   Con OpenMP:  gcc -O2 -fopenmp -o fractal src/*.c -lm
 * 
 * Uso:
 *   ./fractal --width 1920 --height 1080 --mode all --threads 8
 * 
 * Autor: Proyecto Programación Paralela
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "timer.h"
#include "image_io.h"
#include "mandelbrot.h"
#include "convolution.h"
#include "histogram.h"

#ifdef _OPENMP
    #include <omp.h>
#endif

/* Para crear directorios en diferentes plataformas */
#ifdef _WIN32
    #include <direct.h>
    #define MKDIR(dir) _mkdir(dir)
    #define PATH_SEP '\\'
#else
    #include <sys/stat.h>
    #include <sys/types.h>
    #define MKDIR(dir) mkdir(dir, 0755)
    #define PATH_SEP '/'
#endif

/* ============================================================================
 * Enumeraciones para los modos de ejecución
 * ============================================================================ */

/** Modo de ejecución general */
typedef enum {
    MODE_SEQUENTIAL,    /**< Solo versión secuencial */
    MODE_PARALLEL,      /**< Solo versión paralela */
    MODE_VECTORIZED,    /**< Solo versión con SIMD (convolución) */
    MODE_ALL            /**< Todas las versiones + comparación */
} RunMode;

/** Modo de cálculo de histograma */
typedef enum {
    HIST_MODE_SEQUENTIAL,     /**< Sin paralelización */
    HIST_MODE_ATOMIC,         /**< Con #pragma omp atomic */
    HIST_MODE_CRITICAL,       /**< Con #pragma omp critical */
    HIST_MODE_REDUCTION,      /**< Con reducción manual (óptima) */
    HIST_MODE_FALSE_SHARING   /**< Con compartición falsa (mala) */
} HistMode;

/** Planificador OpenMP */
typedef enum {
    SCHED_STATIC,
    SCHED_DYNAMIC,
    SCHED_GUIDED
} SchedType;

/* ============================================================================
 * Estructura de configuración del programa
 * ============================================================================ */
typedef struct {
    int width;              /**< Ancho de la imagen */
    int height;             /**< Alto de la imagen */
    int max_iter;           /**< Iteraciones máximas de Mandelbrot */
    int blur_radius;        /**< Radio del desenfoque gaussiano */
    int num_threads;        /**< Número de hilos (0 = automático) */
    SchedType scheduler;    /**< Planificador OpenMP */
    int chunk_size;         /**< Tamaño de chunk (0 = automático) */
    HistMode hist_mode;     /**< Modo de histograma */
    RunMode run_mode;       /**< Modo de ejecución */
    char output_dir[256];   /**< Directorio de salida */
    int skip_mandelbrot;    /**< Saltar generación de Mandelbrot */
    int skip_filter;        /**< Saltar filtro de convolución */
    int skip_histogram;     /**< Saltar cálculo de histograma */
} Config;

/* ============================================================================
 * Funciones auxiliares
 * ============================================================================ */

/**
 * print_usage - Muestra el mensaje de ayuda con todos los argumentos.
 */
static void print_usage(const char *prog_name) {
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════════╗\n");
    printf("║  Fractal Mandelbrot + Convolucion Gaussiana                    ║\n");
    printf("║  Proyecto de Programacion Paralela con OpenMP                  ║\n");
    printf("╚══════════════════════════════════════════════════════════════════╝\n");
    printf("\nUso: %s [opciones]\n\n", prog_name);
    printf("Opciones:\n");
    printf("  --width W          Ancho de imagen (defecto: %d)\n", DEFAULT_WIDTH);
    printf("  --height H         Alto de imagen (defecto: %d)\n", DEFAULT_HEIGHT);
    printf("  --max-iter N       Max iteraciones Mandelbrot (defecto: %d)\n", MAX_ITER);
    printf("  --blur-radius R    Radio del desenfoque gaussiano (defecto: %d)\n", DEFAULT_BLUR_RADIUS);
    printf("  --threads T        Numero de hilos (defecto: automatico)\n");
    printf("  --scheduler S      Planificador OpenMP: static|dynamic|guided (defecto: static)\n");
    printf("  --chunk C          Tamano de chunk para planificador (defecto: 0 = auto)\n");
    printf("  --histogram-mode M Modo de histograma:\n");
    printf("                       sequential|atomic|critical|reduction|false_sharing\n");
    printf("  --mode M           Modo de ejecucion:\n");
    printf("                       sequential|parallel|vectorized|all (defecto: all)\n");
    printf("  --output-dir D     Directorio de salida (defecto: output)\n");
    printf("  --skip-mandelbrot  Saltar generacion de Mandelbrot\n");
    printf("  --skip-filter      Saltar filtro de convolucion\n");
    printf("  --skip-histogram   Saltar calculo de histograma\n");
    printf("  --help             Mostrar esta ayuda\n");
    printf("\nEjemplos:\n");
    printf("  %s --width 1920 --height 1080 --mode all\n", prog_name);
    printf("  %s --threads 8 --scheduler dynamic --chunk 10\n", prog_name);
    printf("  %s --mode parallel --histogram-mode reduction\n", prog_name);
    printf("\n");
}

/**
 * init_config - Inicializa la configuración con valores por defecto.
 */
static void init_config(Config *cfg) {
    cfg->width          = DEFAULT_WIDTH;
    cfg->height         = DEFAULT_HEIGHT;
    cfg->max_iter       = MAX_ITER;
    cfg->blur_radius    = DEFAULT_BLUR_RADIUS;
    cfg->num_threads    = 0;  /* 0 = automático */
    cfg->scheduler      = SCHED_STATIC;
    cfg->chunk_size     = 0;  /* 0 = automático */
    cfg->hist_mode      = HIST_MODE_REDUCTION;
    cfg->run_mode       = MODE_ALL;
    strcpy(cfg->output_dir, "output");
    cfg->skip_mandelbrot = 0;
    cfg->skip_filter     = 0;
    cfg->skip_histogram  = 0;
}

/**
 * parse_args - Parsea los argumentos de línea de comandos.
 * 
 * @return 0 en éxito, -1 si se solicita ayuda o hay error.
 */
static int parse_args(int argc, char *argv[], Config *cfg) {
    int i;
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return -1;
        }
        else if (strcmp(argv[i], "--width") == 0 && i + 1 < argc) {
            cfg->width = atoi(argv[++i]);
            if (cfg->width <= 0) {
                fprintf(stderr, "[ERROR] Ancho inválido: %s\n", argv[i]);
                return -1;
            }
        }
        else if (strcmp(argv[i], "--height") == 0 && i + 1 < argc) {
            cfg->height = atoi(argv[++i]);
            if (cfg->height <= 0) {
                fprintf(stderr, "[ERROR] Alto inválido: %s\n", argv[i]);
                return -1;
            }
        }
        else if (strcmp(argv[i], "--max-iter") == 0 && i + 1 < argc) {
            cfg->max_iter = atoi(argv[++i]);
            if (cfg->max_iter <= 0) {
                fprintf(stderr, "[ERROR] max-iter inválido: %s\n", argv[i]);
                return -1;
            }
        }
        else if (strcmp(argv[i], "--blur-radius") == 0 && i + 1 < argc) {
            cfg->blur_radius = atoi(argv[++i]);
            if (cfg->blur_radius <= 0) {
                fprintf(stderr, "[ERROR] blur-radius inválido: %s\n", argv[i]);
                return -1;
            }
        }
        else if (strcmp(argv[i], "--threads") == 0 && i + 1 < argc) {
            cfg->num_threads = atoi(argv[++i]);
            if (cfg->num_threads < 0) {
                fprintf(stderr, "[ERROR] threads inválido: %s\n", argv[i]);
                return -1;
            }
        }
        else if (strcmp(argv[i], "--scheduler") == 0 && i + 1 < argc) {
            i++;
            if (strcmp(argv[i], "static") == 0) {
                cfg->scheduler = SCHED_STATIC;
            } else if (strcmp(argv[i], "dynamic") == 0) {
                cfg->scheduler = SCHED_DYNAMIC;
            } else if (strcmp(argv[i], "guided") == 0) {
                cfg->scheduler = SCHED_GUIDED;
            } else {
                fprintf(stderr, "[ERROR] Planificador desconocido: %s\n", argv[i]);
                return -1;
            }
        }
        else if (strcmp(argv[i], "--chunk") == 0 && i + 1 < argc) {
            cfg->chunk_size = atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "--histogram-mode") == 0 && i + 1 < argc) {
            i++;
            if (strcmp(argv[i], "sequential") == 0) {
                cfg->hist_mode = HIST_MODE_SEQUENTIAL;
            } else if (strcmp(argv[i], "atomic") == 0) {
                cfg->hist_mode = HIST_MODE_ATOMIC;
            } else if (strcmp(argv[i], "critical") == 0) {
                cfg->hist_mode = HIST_MODE_CRITICAL;
            } else if (strcmp(argv[i], "reduction") == 0) {
                cfg->hist_mode = HIST_MODE_REDUCTION;
            } else if (strcmp(argv[i], "false_sharing") == 0) {
                cfg->hist_mode = HIST_MODE_FALSE_SHARING;
            } else {
                fprintf(stderr, "[ERROR] Modo de histograma desconocido: %s\n", argv[i]);
                return -1;
            }
        }
        else if (strcmp(argv[i], "--mode") == 0 && i + 1 < argc) {
            i++;
            if (strcmp(argv[i], "sequential") == 0) {
                cfg->run_mode = MODE_SEQUENTIAL;
            } else if (strcmp(argv[i], "parallel") == 0) {
                cfg->run_mode = MODE_PARALLEL;
            } else if (strcmp(argv[i], "vectorized") == 0) {
                cfg->run_mode = MODE_VECTORIZED;
            } else if (strcmp(argv[i], "all") == 0) {
                cfg->run_mode = MODE_ALL;
            } else {
                fprintf(stderr, "[ERROR] Modo de ejecución desconocido: %s\n", argv[i]);
                return -1;
            }
        }
        else if (strcmp(argv[i], "--output-dir") == 0 && i + 1 < argc) {
            strncpy(cfg->output_dir, argv[++i], sizeof(cfg->output_dir) - 1);
            cfg->output_dir[sizeof(cfg->output_dir) - 1] = '\0';
        }
        else if (strcmp(argv[i], "--skip-mandelbrot") == 0) {
            cfg->skip_mandelbrot = 1;
        }
        else if (strcmp(argv[i], "--skip-filter") == 0) {
            cfg->skip_filter = 1;
        }
        else if (strcmp(argv[i], "--skip-histogram") == 0) {
            cfg->skip_histogram = 1;
        }
        else {
            fprintf(stderr, "[ADVERTENCIA] Argumento desconocido: %s\n", argv[i]);
        }
    }
    return 0;
}

/**
 * get_scheduler_name - Devuelve el nombre del planificador como cadena.
 */
static const char* get_scheduler_name(SchedType sched) {
    switch (sched) {
        case SCHED_STATIC:  return "static";
        case SCHED_DYNAMIC: return "dynamic";
        case SCHED_GUIDED:  return "guided";
        default:            return "desconocido";
    }
}

/**
 * get_hist_mode_name - Devuelve el nombre del modo de histograma.
 */
static const char* get_hist_mode_name(HistMode mode) {
    switch (mode) {
        case HIST_MODE_SEQUENTIAL:    return "sequential";
        case HIST_MODE_ATOMIC:        return "atomic";
        case HIST_MODE_CRITICAL:      return "critical";
        case HIST_MODE_REDUCTION:     return "reduction";
        case HIST_MODE_FALSE_SHARING: return "false_sharing";
        default:                      return "desconocido";
    }
}

/**
 * get_run_mode_name - Devuelve el nombre del modo de ejecución.
 */
static const char* get_run_mode_name(RunMode mode) {
    switch (mode) {
        case MODE_SEQUENTIAL: return "sequential";
        case MODE_PARALLEL:   return "parallel";
        case MODE_VECTORIZED: return "vectorized";
        case MODE_ALL:        return "all";
        default:              return "desconocido";
    }
}

/**
 * print_config - Imprime la configuración actual del programa.
 */
static void print_config(const Config *cfg) {
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════════╗\n");
    printf("║              CONFIGURACION DEL PROGRAMA                        ║\n");
    printf("╠══════════════════════════════════════════════════════════════════╣\n");
    printf("║  Resolucion        : %d x %d pixeles\n", cfg->width, cfg->height);
    printf("║  Total pixeles     : %lld\n", (long long)cfg->width * cfg->height);
    printf("║  Memoria imagen    : %.2f MB\n",
           (double)cfg->width * cfg->height * 3 / (1024.0 * 1024.0));
    printf("║  Max iteraciones   : %d\n", cfg->max_iter);
    printf("║  Radio desenfoque  : %d (kernel %dx%d)\n",
           cfg->blur_radius, 2 * cfg->blur_radius + 1, 2 * cfg->blur_radius + 1);
    printf("║  Sigma gaussiana   : %.4f\n", (double)cfg->blur_radius / 3.0);

    #ifdef _OPENMP
    printf("║  OpenMP            : HABILITADO\n");
    printf("║  Hilos disponibles : %d\n",
           cfg->num_threads > 0 ? cfg->num_threads : omp_get_max_threads());
    printf("║  Planificador      : %s", get_scheduler_name(cfg->scheduler));
    if (cfg->chunk_size > 0) {
        printf(" (chunk=%d)", cfg->chunk_size);
    } else {
        printf(" (chunk=auto)");
    }
    printf("\n");
    #else
    printf("║  OpenMP            : DESHABILITADO\n");
    printf("║  Hilos             : 1 (secuencial)\n");
    #endif

    printf("║  Modo ejecucion    : %s\n", get_run_mode_name(cfg->run_mode));
    printf("║  Modo histograma   : %s\n", get_hist_mode_name(cfg->hist_mode));
    printf("║  Directorio salida : %s\n", cfg->output_dir);
    printf("║  Saltar Mandelbrot : %s\n", cfg->skip_mandelbrot ? "SI" : "NO");
    printf("║  Saltar filtro     : %s\n", cfg->skip_filter ? "SI" : "NO");
    printf("║  Saltar histograma : %s\n", cfg->skip_histogram ? "SI" : "NO");
    printf("╚══════════════════════════════════════════════════════════════════╝\n\n");
}

/**
 * configure_openmp - Configura OpenMP según la configuración del usuario.
 */
static void configure_openmp(const Config *cfg) {
    #ifdef _OPENMP
    /* Establecer número de hilos */
    if (cfg->num_threads > 0) {
        omp_set_num_threads(cfg->num_threads);
    }

    /* Establecer planificador en tiempo de ejecución */
    omp_sched_t omp_sched;
    switch (cfg->scheduler) {
        case SCHED_DYNAMIC: omp_sched = omp_sched_dynamic; break;
        case SCHED_GUIDED:  omp_sched = omp_sched_guided;  break;
        default:            omp_sched = omp_sched_static;   break;
    }

    /* 
     * omp_set_schedule() configura el planificador para todas las
     * directivas que usen schedule(runtime).
     * El chunk_size=0 indica que OpenMP debe elegir automáticamente.
     */
    omp_set_schedule(omp_sched, cfg->chunk_size);

    printf("[INFO] OpenMP configurado: %d hilos, planificador=%s",
           omp_get_max_threads(), get_scheduler_name(cfg->scheduler));
    if (cfg->chunk_size > 0) {
        printf(", chunk=%d", cfg->chunk_size);
    }
    printf("\n\n");
    #else
    (void)cfg;  /* Suprimir advertencia de parámetro no usado */
    printf("[INFO] Compilado SIN OpenMP. Ejecutando en modo secuencial.\n\n");
    #endif
}

/**
 * build_filepath - Construye la ruta completa de un archivo de salida.
 */
static void build_filepath(char *buf, size_t buf_size,
                           const char *dir, const char *filename) {
    snprintf(buf, buf_size, "%s%c%s", dir, PATH_SEP, filename);
}

/**
 * run_histogram - Ejecuta el cálculo de histograma con la estrategia seleccionada.
 */
static double run_histogram(const Image *img, HistMode mode,
                            long long hist_r[HIST_SIZE],
                            long long hist_g[HIST_SIZE],
                            long long hist_b[HIST_SIZE]) {
    double t_start = timer_start();

    switch (mode) {
        case HIST_MODE_SEQUENTIAL:
            compute_histogram_sequential(img, hist_r, hist_g, hist_b);
            break;
        case HIST_MODE_ATOMIC:
            compute_histogram_atomic(img, hist_r, hist_g, hist_b);
            break;
        case HIST_MODE_CRITICAL:
            compute_histogram_critical(img, hist_r, hist_g, hist_b);
            break;
        case HIST_MODE_REDUCTION:
            compute_histogram_reduction(img, hist_r, hist_g, hist_b);
            break;
        case HIST_MODE_FALSE_SHARING:
            compute_histogram_false_sharing(img, hist_r, hist_g, hist_b);
            break;
    }

    double t_stop = timer_stop();
    return timer_elapsed(t_start, t_stop);
}

/* ============================================================================
 * FUNCIÓN PRINCIPAL
 * ============================================================================ */
int main(int argc, char *argv[]) {
    /* ========================================
     * 1. Parsear argumentos de línea de comandos
     * ======================================== */
    Config cfg;
    init_config(&cfg);

    if (parse_args(argc, argv, &cfg) != 0) {
        return 1;
    }

    /* ========================================
     * 2. Imprimir configuración
     * ======================================== */
    printf("\n");
    printf("================================================================\n");
    printf("   FRACTAL MANDELBROT + CONVOLUCION GAUSSIANA\n");
    printf("   Proyecto de Programacion Paralela con OpenMP\n");
    printf("================================================================\n");

    print_config(&cfg);

    /* ========================================
     * 3. Configurar OpenMP
     * ======================================== */
    configure_openmp(&cfg);

    /* ========================================
     * 4. Crear directorio de salida
     * ======================================== */
    MKDIR(cfg.output_dir);

    /* Variables para acumular tiempos */
    double t_mandelbrot_seq = 0.0, t_mandelbrot_par = 0.0;
    double t_conv_seq = 0.0, t_conv_par = 0.0, t_conv_simd = 0.0;
    double t_hist = 0.0;
    double t_hist_all[5] = {0.0, 0.0, 0.0, 0.0, 0.0};  /* Para modo ALL */

    /* Buffers para rutas de archivos */
    char filepath[512];

    /* ========================================
     * 5. Generación del fractal de Mandelbrot
     * ======================================== */
    Image *mandelbrot_img = NULL;

    if (!cfg.skip_mandelbrot) {
        printf("════════════════════════════════════════════════════════════════\n");
        printf("  ETAPA 1: GENERACION DEL FRACTAL DE MANDELBROT\n");
        printf("════════════════════════════════════════════════════════════════\n\n");

        if (cfg.run_mode == MODE_SEQUENTIAL || cfg.run_mode == MODE_ALL) {
            /* --- Versión secuencial --- */
            printf("  [SEQ] Generando Mandelbrot secuencial (%dx%d, %d iter)...\n",
                   cfg.width, cfg.height, cfg.max_iter);

            Image *img_seq = create_image(cfg.width, cfg.height);
            if (!img_seq) {
                fprintf(stderr, "[ERROR] No se pudo crear la imagen\n");
                return 1;
            }

            double t0 = timer_start();
            generate_mandelbrot(img_seq,
                               DEFAULT_X_MIN, DEFAULT_X_MAX,
                               DEFAULT_Y_MIN, DEFAULT_Y_MAX,
                               cfg.max_iter);
            double t1 = timer_stop();
            t_mandelbrot_seq = timer_elapsed(t0, t1);
            timer_print("Mandelbrot secuencial", t_mandelbrot_seq);

            /* Guardar imagen secuencial */
            build_filepath(filepath, sizeof(filepath), cfg.output_dir, "mandelbrot_seq.ppm");
            if (write_ppm(filepath, img_seq) == 0) {
                printf("  [OK]  Imagen guardada: %s\n", filepath);
            }

            /* Usar esta imagen para las siguientes etapas si no hay paralela */
            if (cfg.run_mode == MODE_SEQUENTIAL) {
                mandelbrot_img = img_seq;
            } else {
                free_image(img_seq);
            }
        }

        if (cfg.run_mode == MODE_PARALLEL || cfg.run_mode == MODE_VECTORIZED || cfg.run_mode == MODE_ALL) {
            /* --- Versión paralela --- */
            printf("  [PAR] Generando Mandelbrot paralelo (%dx%d, %d iter)...\n",
                   cfg.width, cfg.height, cfg.max_iter);

            Image *img_par = create_image(cfg.width, cfg.height);
            if (!img_par) {
                fprintf(stderr, "[ERROR] No se pudo crear la imagen\n");
                return 1;
            }

            double t0 = timer_start();
            generate_mandelbrot_parallel(img_par,
                                         DEFAULT_X_MIN, DEFAULT_X_MAX,
                                         DEFAULT_Y_MIN, DEFAULT_Y_MAX,
                                         cfg.max_iter);
            double t1 = timer_stop();
            t_mandelbrot_par = timer_elapsed(t0, t1);
            timer_print("Mandelbrot paralelo", t_mandelbrot_par);

            /* Guardar imagen paralela */
            build_filepath(filepath, sizeof(filepath), cfg.output_dir, "mandelbrot_par.ppm");
            if (write_ppm(filepath, img_par) == 0) {
                printf("  [OK]  Imagen guardada: %s\n", filepath);
            }

            mandelbrot_img = img_par;
        }

        /* Mostrar comparación si se ejecutaron ambas versiones */
        if (cfg.run_mode == MODE_ALL && t_mandelbrot_seq > 0.0 && t_mandelbrot_par > 0.0) {
            printf("\n");
            timer_print_comparison("Mandelbrot", t_mandelbrot_seq, t_mandelbrot_par);
        }

        printf("\n");
    } else {
        /* Intentar cargar imagen existente */
        printf("[INFO] Saltando generación de Mandelbrot, cargando imagen...\n");
        build_filepath(filepath, sizeof(filepath), cfg.output_dir, "mandelbrot_par.ppm");
        mandelbrot_img = read_ppm(filepath);
        if (!mandelbrot_img) {
            build_filepath(filepath, sizeof(filepath), cfg.output_dir, "mandelbrot_seq.ppm");
            mandelbrot_img = read_ppm(filepath);
        }
        if (!mandelbrot_img) {
            fprintf(stderr, "[ERROR] No se pudo cargar ninguna imagen de Mandelbrot\n");
            return 1;
        }
        printf("[OK]   Imagen cargada: %s (%dx%d)\n\n",
               filepath, mandelbrot_img->width, mandelbrot_img->height);
    }

    /* ========================================
     * 6. Convolución Gaussiana
     * ======================================== */
    Image *filtered_img = NULL;

    if (!cfg.skip_filter && mandelbrot_img) {
        printf("════════════════════════════════════════════════════════════════\n");
        printf("  ETAPA 2: CONVOLUCION GAUSSIANA (Desenfoque)\n");
        printf("════════════════════════════════════════════════════════════════\n\n");

        /* Crear kernel gaussiano */
        double sigma = (double)cfg.blur_radius / 3.0;
        Kernel *kernel = create_gaussian_kernel(cfg.blur_radius, sigma);
        if (!kernel) {
            fprintf(stderr, "[ERROR] No se pudo crear el kernel gaussiano\n");
            free_image(mandelbrot_img);
            return 1;
        }

        int ksize = 2 * cfg.blur_radius + 1;
        printf("  [INFO] Kernel gaussiano: %dx%d, sigma=%.4f\n", ksize, ksize, sigma);

        if (cfg.run_mode == MODE_SEQUENTIAL || cfg.run_mode == MODE_ALL) {
            /* --- Versión secuencial --- */
            printf("  [SEQ] Aplicando convolución secuencial...\n");

            Image *out_seq = create_image(mandelbrot_img->width, mandelbrot_img->height);
            if (!out_seq) {
                fprintf(stderr, "[ERROR] No se pudo crear imagen de salida\n");
                free_kernel(kernel);
                free_image(mandelbrot_img);
                return 1;
            }

            double t0 = timer_start();
            apply_convolution(mandelbrot_img, out_seq, kernel);
            double t1 = timer_stop();
            t_conv_seq = timer_elapsed(t0, t1);
            timer_print("Convolución secuencial", t_conv_seq);

            build_filepath(filepath, sizeof(filepath), cfg.output_dir, "filtered_seq.ppm");
            if (write_ppm(filepath, out_seq) == 0) {
                printf("  [OK]  Imagen guardada: %s\n", filepath);
            }

            if (cfg.run_mode == MODE_SEQUENTIAL) {
                filtered_img = out_seq;
            } else {
                free_image(out_seq);
            }
        }

        if (cfg.run_mode == MODE_PARALLEL || cfg.run_mode == MODE_ALL) {
            /* --- Versión paralela --- */
            printf("  [PAR] Aplicando convolución paralela...\n");

            Image *out_par = create_image(mandelbrot_img->width, mandelbrot_img->height);
            if (!out_par) {
                fprintf(stderr, "[ERROR] No se pudo crear imagen de salida\n");
                free_kernel(kernel);
                free_image(mandelbrot_img);
                return 1;
            }

            double t0 = timer_start();
            apply_convolution_parallel(mandelbrot_img, out_par, kernel);
            double t1 = timer_stop();
            t_conv_par = timer_elapsed(t0, t1);
            timer_print("Convolución paralela", t_conv_par);

            build_filepath(filepath, sizeof(filepath), cfg.output_dir, "filtered_par.ppm");
            if (write_ppm(filepath, out_par) == 0) {
                printf("  [OK]  Imagen guardada: %s\n", filepath);
            }

            if (cfg.run_mode != MODE_ALL) {
                filtered_img = out_par;
            } else {
                free_image(out_par);
            }
        }

        if (cfg.run_mode == MODE_VECTORIZED || cfg.run_mode == MODE_ALL) {
            /* --- Versión SIMD --- */
            printf("  [SIMD] Aplicando convolución con vectorización SIMD...\n");

            Image *out_simd = create_image(mandelbrot_img->width, mandelbrot_img->height);
            if (!out_simd) {
                fprintf(stderr, "[ERROR] No se pudo crear imagen de salida\n");
                free_kernel(kernel);
                free_image(mandelbrot_img);
                return 1;
            }

            double t0 = timer_start();
            apply_convolution_simd(mandelbrot_img, out_simd, kernel);
            double t1 = timer_stop();
            t_conv_simd = timer_elapsed(t0, t1);
            timer_print("Convolución SIMD", t_conv_simd);

            build_filepath(filepath, sizeof(filepath), cfg.output_dir, "filtered_simd.ppm");
            if (write_ppm(filepath, out_simd) == 0) {
                printf("  [OK]  Imagen guardada: %s\n", filepath);
            }

            filtered_img = out_simd;
        }

        /* Mostrar comparaciones */
        if (cfg.run_mode == MODE_ALL) {
            printf("\n");
            if (t_conv_seq > 0.0 && t_conv_par > 0.0) {
                timer_print_comparison("Convolución (Seq vs Par)", t_conv_seq, t_conv_par);
            }
            if (t_conv_seq > 0.0 && t_conv_simd > 0.0) {
                timer_print_comparison("Convolución (Seq vs SIMD)", t_conv_seq, t_conv_simd);
            }
        }

        free_kernel(kernel);
        printf("\n");
    }

    /* ========================================
     * 7. Cálculo de Histogramas
     * ======================================== */
    /* Usar la imagen filtrada si existe, sino la de Mandelbrot */
    Image *hist_img = filtered_img ? filtered_img : mandelbrot_img;

    if (!cfg.skip_histogram && hist_img) {
        printf("════════════════════════════════════════════════════════════════\n");
        printf("  ETAPA 3: CALCULO DE HISTOGRAMAS DE COLOR\n");
        printf("════════════════════════════════════════════════════════════════\n\n");

        long long hist_r[HIST_SIZE], hist_g[HIST_SIZE], hist_b[HIST_SIZE];

        if (cfg.run_mode == MODE_ALL) {
            /* Ejecutar TODAS las estrategias de histograma para comparar */
            const char *mode_names[] = {
                "Secuencial", "Atomic", "Critical", "Reduction", "False Sharing"
            };
            HistMode modes[] = {
                HIST_MODE_SEQUENTIAL, HIST_MODE_ATOMIC, HIST_MODE_CRITICAL,
                HIST_MODE_REDUCTION, HIST_MODE_FALSE_SHARING
            };

            int m;
            for (m = 0; m < 5; m++) {
                printf("  [HIST] Ejecutando histograma: %s...\n", mode_names[m]);
                t_hist_all[m] = run_histogram(hist_img, modes[m], hist_r, hist_g, hist_b);
                timer_print(mode_names[m], t_hist_all[m]);
            }

            /* Mostrar comparaciones */
            printf("\n  Comparación de estrategias de histograma:\n");
            printf("  ┌─────────────────────┬────────────┬──────────┐\n");
            printf("  │ Estrategia          │ Tiempo (s) │ Relativo │\n");
            printf("  ├─────────────────────┼────────────┼──────────┤\n");
            for (m = 0; m < 5; m++) {
                double relative = (t_hist_all[0] > 0.0) ? t_hist_all[m] / t_hist_all[0] : 0.0;
                printf("  │ %-19s │ %10.4f │ %7.2fx │\n",
                       mode_names[m], t_hist_all[m], relative);
            }
            printf("  └─────────────────────┴────────────┴──────────┘\n");

            t_hist = t_hist_all[3];  /* Usar reduction como referencia */
        } else {
            /* Ejecutar solo la estrategia seleccionada */
            printf("  [HIST] Ejecutando histograma: %s...\n", get_hist_mode_name(cfg.hist_mode));
            t_hist = run_histogram(hist_img, cfg.hist_mode, hist_r, hist_g, hist_b);
            timer_print("Histograma", t_hist);
        }

        /* Imprimir resumen del histograma */
        print_histogram_summary(hist_r, hist_g, hist_b);
    }

    /* ========================================
     * 8. Resumen final de tiempos
     * ======================================== */
    printf("════════════════════════════════════════════════════════════════\n");
    printf("  RESUMEN FINAL DE TIEMPOS\n");
    printf("════════════════════════════════════════════════════════════════\n\n");

    printf("  ┌──────────────────────────────────────┬────────────────┐\n");
    printf("  │ Operación                            │ Tiempo (seg)   │\n");
    printf("  ├──────────────────────────────────────┼────────────────┤\n");

    double total_time = 0.0;

    if (t_mandelbrot_seq > 0.0) {
        printf("  │ Mandelbrot (secuencial)              │ %14.4f │\n", t_mandelbrot_seq);
        total_time += t_mandelbrot_seq;
    }
    if (t_mandelbrot_par > 0.0) {
        printf("  │ Mandelbrot (paralelo)                │ %14.4f │\n", t_mandelbrot_par);
        total_time += t_mandelbrot_par;
    }
    if (t_conv_seq > 0.0) {
        printf("  │ Convolución (secuencial)             │ %14.4f │\n", t_conv_seq);
        total_time += t_conv_seq;
    }
    if (t_conv_par > 0.0) {
        printf("  │ Convolución (paralela)               │ %14.4f │\n", t_conv_par);
        total_time += t_conv_par;
    }
    if (t_conv_simd > 0.0) {
        printf("  │ Convolución (SIMD)                   │ %14.4f │\n", t_conv_simd);
        total_time += t_conv_simd;
    }

    if (cfg.run_mode == MODE_ALL) {
        const char *hist_names[] = {
            "Histograma (secuencial)",
            "Histograma (atomic)",
            "Histograma (critical)",
            "Histograma (reduction)",
            "Histograma (false_sharing)"
        };
        int m;
        for (m = 0; m < 5; m++) {
            if (t_hist_all[m] > 0.0) {
                printf("  │ %-38s │ %14.4f │\n", hist_names[m], t_hist_all[m]);
                total_time += t_hist_all[m];
            }
        }
    } else if (t_hist > 0.0) {
        char hist_label[64];
        snprintf(hist_label, sizeof(hist_label), "Histograma (%s)", get_hist_mode_name(cfg.hist_mode));
        printf("  │ %-38s │ %14.4f │\n", hist_label, t_hist);
        total_time += t_hist;
    }

    printf("  ├──────────────────────────────────────┼────────────────┤\n");
    printf("  │ TOTAL                                │ %14.4f │\n", total_time);
    printf("  └──────────────────────────────────────┴────────────────┘\n");

    /* Speedup resumen */
    if (cfg.run_mode == MODE_ALL) {
        printf("\n  Resumen de Speedup:\n");
        printf("  ┌──────────────────────────┬────────────┬────────────┬──────────┐\n");
        printf("  │ Operación                │ Seq (s)    │ Par (s)    │ Speedup  │\n");
        printf("  ├──────────────────────────┼────────────┼────────────┼──────────┤\n");

        if (t_mandelbrot_seq > 0.0 && t_mandelbrot_par > 0.0) {
            printf("  │ Mandelbrot               │ %10.4f │ %10.4f │ %7.2fx │\n",
                   t_mandelbrot_seq, t_mandelbrot_par, t_mandelbrot_seq / t_mandelbrot_par);
        }
        if (t_conv_seq > 0.0 && t_conv_par > 0.0) {
            printf("  │ Convolución (paralela)   │ %10.4f │ %10.4f │ %7.2fx │\n",
                   t_conv_seq, t_conv_par, t_conv_seq / t_conv_par);
        }
        if (t_conv_seq > 0.0 && t_conv_simd > 0.0) {
            printf("  │ Convolución (SIMD)       │ %10.4f │ %10.4f │ %7.2fx │\n",
                   t_conv_seq, t_conv_simd, t_conv_seq / t_conv_simd);
        }

        printf("  └──────────────────────────┴────────────┴────────────┴──────────┘\n");
    }

    printf("\n[COMPLETADO] Programa finalizado exitosamente.\n\n");

    /* ========================================
     * 9. Liberar memoria
     * ======================================== */
    if (filtered_img && filtered_img != mandelbrot_img) {
        free_image(filtered_img);
    }
    free_image(mandelbrot_img);

    return 0;
}
