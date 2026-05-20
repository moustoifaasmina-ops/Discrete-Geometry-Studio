/**
 * @file ppm.c
 * @brief Gestion des images PPM (lecture RGB, écriture, conversion)
 *
 * Ce module permet :
 * - allocation/libération d’images PPM
 * - écriture au format P6
 * - conversion de matrices (distance / bissectrice) en images RGB
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "ppm.h"
#include "pgm.h"

/**
 * @brief Alloue une image PPM (RGB)
 *
 * @param height hauteur
 * @param width largeur
 * @return matrice 3D [y][x][rgb]
 */
unsigned char ***allocateMatrixPPM(int height, int width) {
    unsigned char ***m = malloc(height * sizeof(unsigned char **));

    for (int i = 0; i < height; i++) {
        m[i] = malloc(width * sizeof(unsigned char *));
        for (int j = 0; j < width; j++)
            m[i][j] = calloc(3, sizeof(unsigned char));
    }
    return m;
}

/**
 * @brief Libère une image PPM
 *
 * @param matrix image RGB
 * @param height hauteur
 * @param width largeur
 */
void freeMatrixPPM(unsigned char ***matrix, int height, int width) {
    if (!matrix) return;

    for (int i = 0; i < height; i++) {
        if (matrix[i]) {
            for (int j = 0; j < width; j++)
                free(matrix[i][j]);
            free(matrix[i]);
        }
    }
    free(matrix);
}

/**
 * @brief Libère une structure PPMImage
 */
void freePPM(PPMImage img) {
    freeMatrixPPM(img.data, img.height, img.width);
}

/**
 * @brief Écrit une image PPM (format P6)
 */
void writePPM(const char *filename, PPMImage img) {
    FILE *file = fopen(filename, "wb");
    if (!file) {
        perror("Erreur ouverture PPM écriture");
        return;
    }

    fprintf(file, "P6\n%d %d\n%d\n", img.width, img.height, img.max_val);

    for (int i = 0; i < img.height; i++)
        for (int j = 0; j < img.width; j++)
            fwrite(img.data[i][j], 1, 3, file);

    fclose(file);
}

/**
 * @brief Convertit une matrice de distance en image PPM (niveau de gris RGB)
 */
PPMImage distanceToPPMImage(int **distance, int height, int width) {
    PPMImage img;
    img.width = width;
    img.height = height;
    img.max_val = 255;
    img.data = allocateMatrixPPM(height, width);

    int mx = 0;
    for (int i = 0; i < height; i++)
        for (int j = 0; j < width; j++)
            if (distance[i][j] > mx) mx = distance[i][j];

    if (mx == 0) mx = 1;

    for (int i = 0; i < height; i++)
        for (int j = 0; j < width; j++) {
            unsigned char v =
                (unsigned char)(255.0 * distance[i][j] / mx);

            img.data[i][j][0] = v;
            img.data[i][j][1] = v;
            img.data[i][j][2] = v;
        }

    return img;
}

/**
 * @brief Convertit une matrice de bissectrice en image PPM colorée
 *
 * Utilise une colormap type "magma-like"
 */
PPMImage bisectorToPPMImage(double **matrix, int height, int width) {
    PPMImage img;
    img.width = width;
    img.height = height;
    img.max_val = 255;
    img.data = allocateMatrixPPM(height, width);

    double mx = 0;
    for (int i = 0; i < height; i++)
        for (int j = 0; j < width; j++)
            if (matrix[i][j] > mx) mx = matrix[i][j];

    if (mx == 0) mx = 1;

    for (int i = 0; i < height; i++)
        for (int j = 0; j < width; j++) {

            double t = matrix[i][j] / mx;

            /* colormap simple type magma-like */
            unsigned char r = (unsigned char)(255 * (0.5 + 0.5 * t));
            unsigned char g = (unsigned char)(255 * (0.2 * t * t));
            unsigned char b = (unsigned char)(255 * (0.1 + 0.4 * t));

            img.data[i][j][0] = r;
            img.data[i][j][1] = g;
            img.data[i][j][2] = b;
        }

    return img;
}
