/**
 * ============================================================================
 * image_io.c - Implementación de Entrada/Salida de imágenes PPM
 * ============================================================================
 * 
 * Implementa las funciones de creación, liberación, lectura y escritura
 * de imágenes en formato PPM P6 (binario).
 * 
 * El formato PPM P6 es simple y portátil:
 *   - No requiere bibliotecas externas (libpng, libjpeg, etc.)
 *   - Soportado por la mayoría de visores de imagen
 *   - Fácil de leer y escribir programáticamente
 * 
 * Autor: Proyecto Programación Paralela
 * ============================================================================
 */

#include "image_io.h"

/* ============================================================================
 * create_image - Crea una nueva imagen y la inicializa a negro.
 * 
 * Asigna memoria para la estructura Image y el arreglo de pixeles RGB.
 * El arreglo se inicializa a cero (todos los pixeles negros).
 * 
 * Complejidad: O(width * height) para la inicialización con memset.
 * ============================================================================ */
Image* create_image(int width, int height) {
    /* Validar dimensiones */
    if (width <= 0 || height <= 0) {
        fprintf(stderr, "[ERROR] Dimensiones de imagen inválidas: %dx%d\n", width, height);
        return NULL;
    }

    /* Asignar la estructura Image */
    Image *img = (Image*)malloc(sizeof(Image));
    if (!img) {
        fprintf(stderr, "[ERROR] No se pudo asignar memoria para la estructura Image\n");
        return NULL;
    }

    img->width  = width;
    img->height = height;

    /* Calcular tamaño total: 3 bytes por pixel (RGB) */
    size_t data_size = (size_t)width * (size_t)height * 3;

    /* Asignar el arreglo de datos e inicializar a cero (negro) */
    img->data = (unsigned char*)calloc(data_size, sizeof(unsigned char));
    if (!img->data) {
        fprintf(stderr, "[ERROR] No se pudo asignar memoria para datos de imagen (%zu bytes)\n",
                data_size);
        free(img);
        return NULL;
    }

    return img;
}

/* ============================================================================
 * free_image - Libera toda la memoria asociada a una imagen.
 * 
 * Es seguro pasar NULL (no hace nada en ese caso).
 * ============================================================================ */
void free_image(Image *img) {
    if (img) {
        if (img->data) {
            free(img->data);
            img->data = NULL;
        }
        free(img);
    }
}

/* ============================================================================
 * write_ppm - Escribe una imagen en formato PPM P6 (binario).
 * 
 * Formato del archivo:
 *   Línea 1: "P6"             (número mágico PPM binario)
 *   Línea 2: "<ancho> <alto>"  (dimensiones)
 *   Línea 3: "255"            (valor máximo por canal)
 *   Resto:   datos binarios RGB, 3 bytes por pixel, fila por fila
 * 
 * Retorna 0 en éxito, -1 en error.
 * ============================================================================ */
int write_ppm(const char *filename, const Image *img) {
    /* Validar parámetros */
    if (!filename || !img || !img->data) {
        fprintf(stderr, "[ERROR] Parámetros inválidos para write_ppm\n");
        return -1;
    }

    /* Abrir archivo en modo binario */
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        fprintf(stderr, "[ERROR] No se pudo abrir el archivo para escritura: %s\n", filename);
        return -1;
    }

    /* Escribir encabezado PPM P6 */
    fprintf(fp, "P6\n");
    fprintf(fp, "# Generado por Proyecto Programacion Paralela - Mandelbrot + Convolucion\n");
    fprintf(fp, "%d %d\n", img->width, img->height);
    fprintf(fp, "255\n");

    /* Escribir datos de pixeles en binario */
    size_t total_pixels = (size_t)img->width * (size_t)img->height;
    size_t pixels_written = fwrite(img->data, 3, total_pixels, fp);

    if (pixels_written != total_pixels) {
        fprintf(stderr, "[ERROR] Error al escribir datos de imagen: se escribieron %zu de %zu pixeles\n",
                pixels_written, total_pixels);
        fclose(fp);
        return -1;
    }

    fclose(fp);
    return 0;
}

/* ============================================================================
 * read_ppm - Lee una imagen desde un archivo PPM P6.
 * 
 * Soporta comentarios (líneas que comienzan con '#') en el encabezado.
 * Solo se acepta el formato P6 (binario) con valor máximo 255.
 * 
 * Retorna un puntero a la imagen leída, o NULL si falla.
 * ============================================================================ */
Image* read_ppm(const char *filename) {
    if (!filename) {
        fprintf(stderr, "[ERROR] Nombre de archivo nulo en read_ppm\n");
        return NULL;
    }

    /* Abrir archivo en modo binario */
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        fprintf(stderr, "[ERROR] No se pudo abrir el archivo para lectura: %s\n", filename);
        return NULL;
    }

    /* Leer número mágico */
    char magic[3];
    if (fscanf(fp, "%2s", magic) != 1) {
        fprintf(stderr, "[ERROR] No se pudo leer el número mágico de: %s\n", filename);
        fclose(fp);
        return NULL;
    }

    /* Verificar que sea P6 */
    if (strcmp(magic, "P6") != 0) {
        fprintf(stderr, "[ERROR] Formato no soportado '%s' (solo se acepta P6): %s\n",
                magic, filename);
        fclose(fp);
        return NULL;
    }

    /* Leer dimensiones, saltando comentarios */
    int width, height, max_val;
    int c;

    /* Función auxiliar: saltar espacios en blanco y comentarios */
    /* Saltar espacios y comentarios antes del ancho */
    while ((c = fgetc(fp)) != EOF) {
        if (c == '#') {
            /* Saltar el resto de la línea de comentario */
            while ((c = fgetc(fp)) != EOF && c != '\n');
        } else if (c > ' ') {
            /* Carácter no-espacio encontrado, devolver al flujo */
            ungetc(c, fp);
            break;
        }
    }

    if (fscanf(fp, "%d", &width) != 1) {
        fprintf(stderr, "[ERROR] No se pudo leer el ancho de: %s\n", filename);
        fclose(fp);
        return NULL;
    }

    /* Saltar espacios y comentarios antes del alto */
    while ((c = fgetc(fp)) != EOF) {
        if (c == '#') {
            while ((c = fgetc(fp)) != EOF && c != '\n');
        } else if (c > ' ') {
            ungetc(c, fp);
            break;
        }
    }

    if (fscanf(fp, "%d", &height) != 1) {
        fprintf(stderr, "[ERROR] No se pudo leer el alto de: %s\n", filename);
        fclose(fp);
        return NULL;
    }

    /* Saltar espacios y comentarios antes del valor máximo */
    while ((c = fgetc(fp)) != EOF) {
        if (c == '#') {
            while ((c = fgetc(fp)) != EOF && c != '\n');
        } else if (c > ' ') {
            ungetc(c, fp);
            break;
        }
    }

    if (fscanf(fp, "%d", &max_val) != 1) {
        fprintf(stderr, "[ERROR] No se pudo leer el valor máximo de: %s\n", filename);
        fclose(fp);
        return NULL;
    }

    /* Verificar valor máximo */
    if (max_val != 255) {
        fprintf(stderr, "[ERROR] Valor máximo %d no soportado (solo 255): %s\n",
                max_val, filename);
        fclose(fp);
        return NULL;
    }

    /* Leer exactamente un carácter de espacio en blanco después del max_val */
    fgetc(fp);

    /* Crear imagen y leer datos binarios */
    Image *img = create_image(width, height);
    if (!img) {
        fclose(fp);
        return NULL;
    }

    size_t total_pixels = (size_t)width * (size_t)height;
    size_t pixels_read = fread(img->data, 3, total_pixels, fp);

    if (pixels_read != total_pixels) {
        fprintf(stderr, "[ERROR] Error al leer datos: se leyeron %zu de %zu pixeles: %s\n",
                pixels_read, total_pixels, filename);
        free_image(img);
        fclose(fp);
        return NULL;
    }

    fclose(fp);
    return img;
}
