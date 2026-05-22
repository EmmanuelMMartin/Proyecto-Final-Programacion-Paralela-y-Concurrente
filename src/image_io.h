/**
 * ============================================================================
 * image_io.h - Entrada/Salida de imágenes en formato PPM
 * ============================================================================
 * 
 * Proporciona funciones para crear, liberar, leer y escribir imágenes
 * utilizando el formato PPM P6 (binario). Este formato no requiere
 * bibliotecas externas y es ampliamente soportado.
 * 
 * Formato PPM P6:
 *   - Encabezado de texto: "P6\n<ancho> <alto>\n<max_val>\n"
 *   - Datos binarios: RGB intercalado, 3 bytes por pixel
 * 
 * Autor: Proyecto Programación Paralela
 * ============================================================================
 */

#ifndef IMAGE_IO_H
#define IMAGE_IO_H

#include "common.h"

/**
 * create_image - Crea una nueva imagen con las dimensiones especificadas.
 * 
 * Asigna memoria para los datos de la imagen (width * height * 3 bytes)
 * e inicializa todos los pixeles a negro (0, 0, 0).
 * 
 * @param width  Ancho de la imagen en pixeles.
 * @param height Alto de la imagen en pixeles.
 * @return Puntero a la imagen creada, o NULL si falla la asignación.
 */
Image* create_image(int width, int height);

/**
 * free_image - Libera la memoria asociada a una imagen.
 * 
 * Libera tanto el arreglo de datos como la estructura Image.
 * Es seguro pasar NULL.
 * 
 * @param img Puntero a la imagen a liberar.
 */
void free_image(Image *img);

/**
 * write_ppm - Escribe una imagen al disco en formato PPM P6 (binario).
 * 
 * @param filename Ruta del archivo de salida.
 * @param img      Puntero a la imagen a escribir.
 * @return 0 en éxito, -1 en error.
 */
int write_ppm(const char *filename, const Image *img);

/**
 * read_ppm - Lee una imagen desde un archivo PPM P6 (binario).
 * 
 * Soporta comentarios (líneas que comienzan con '#') en el encabezado.
 * 
 * @param filename Ruta del archivo a leer.
 * @return Puntero a la imagen leída, o NULL si falla.
 */
Image* read_ppm(const char *filename);

#endif /* IMAGE_IO_H */
