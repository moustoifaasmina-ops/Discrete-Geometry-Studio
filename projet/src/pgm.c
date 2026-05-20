/**
 * @file pgm.c
 * @brief Gestion des images PGM (lecture, écriture, conversion, binarisation)
 *
 * Ce module implémente :
 * - lecture de fichiers PGM (format P5)
 * - écriture de fichiers PGM
 * - allocation et libération mémoire
 * - binarisation d’image
 * - conversion de matrices de distance en image
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include "pgm.h"

/**
 * @brief Lit une image PGM (format P5)
 *
 * @param filename chemin du fichier PGM
 * @return PGMImage structure contenant l'image
 */
PGMImage readPGM(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) { perror("Erreur ouverture PGM"); exit(EXIT_FAILURE); }

    char magic[3];
    if (fscanf(file, "%2s", magic) != 1 || strcmp(magic, "P5") != 0) {
        fprintf(stderr, "Format PGM non supporté (P5 attendu)\n");
        fclose(file); exit(EXIT_FAILURE);
    }

    int width = 0, height = 0, max_val = 0;
    char line[256];
    int valuesRead = 0;

    while (valuesRead < 3) {
        if (!fgets(line, sizeof(line), file)) {
            fprintf(stderr, "Erreur lecture en-tête PGM\n");
            fclose(file); exit(EXIT_FAILURE);
        }
        if (line[0] == '#') continue;

        char *ptr = line;
        while (valuesRead < 3) {
            while (isspace(*ptr)) ptr++;
            if (*ptr == '\0' || *ptr == '#') break;

            int val, n;
            if (sscanf(ptr, "%d%n", &val, &n) == 1) {
                if (valuesRead == 0) width = val;
                else if (valuesRead == 1) height = val;
                else max_val = val;
                valuesRead++;
                ptr += n;
            } else break;
        }
    }

    if (width <= 0 || height <= 0 || max_val <= 0 || max_val > 255) {
        fprintf(stderr, "En-tête PGM invalide\n");
        fclose(file); exit(EXIT_FAILURE);
    }

    int c = fgetc(file);
    if (c != '\n' && c != '\r') ungetc(c, file);

    PGMImage img;
    img.width = width;
    img.height = height;
    img.max_val = max_val;
    img.data = allocateMatrix(height, width);

    for (int i = 0; i < height; i++) {
        if (fread(img.data[i], 1, width, file) != (size_t)width) {
            fprintf(stderr, "Erreur lecture ligne %d\n", i);
            fclose(file); exit(EXIT_FAILURE);
        }
    }

    fclose(file);
    return img;
}

/**
 * @brief Écrit une image PGM (format P5)
 *
 * @param filename fichier de sortie
 * @param img image à écrire
 */
void writePGM(const char *filename, PGMImage img) {
    FILE *file = fopen(filename, "wb");
    if (!file) { perror("Erreur ouverture PGM écriture"); return; }

    fprintf(file, "P5\n%d %d\n%d\n", img.width, img.height, img.max_val);

    for (int i = 0; i < img.height; i++)
        fwrite(img.data[i], 1, img.width, file);

    fclose(file);
}

/**
 * @brief Alloue une matrice 2D de pixels
 *
 * @param height hauteur
 * @param width largeur
 * @return matrice allouée
 */
unsigned char **allocateMatrix(int height, int width) {
    unsigned char **m = malloc(height * sizeof(unsigned char *));
    if (!m) return NULL;

    for (int i = 0; i < height; i++) {
        m[i] = calloc(width, sizeof(unsigned char));
        if (!m[i]) {
            for (int j = 0; j < i; j++) free(m[j]);
            free(m);
            return NULL;
        }
    }
    return m;
}

/**
 * @brief Libère une matrice PGM
 */
void freeMatrix(unsigned char **matrix, int height) {
    if (!matrix) return;
    for (int i = 0; i < height; i++) free(matrix[i]);
    free(matrix);
}

/**
 * @brief Libère une image PGM
 */
void freePGM(PGMImage img) {
    freeMatrix(img.data, img.height);
}

/**
 * @brief Binarise une image PGM
 *
 * @param img image
 * @param threshold seuil de binarisation
 */
void binarizeImage(PGMImage img, unsigned char threshold) {
    for (int y = 0; y < img.height; y++)
        for (int x = 0; x < img.width; x++)
            img.data[y][x] = img.data[y][x] > threshold ? 255 : 0;
}

/**
 * @brief Convertit une carte de distance en image PGM normalisée
 */
PGMImage distanceToPGMImage(int **distance, int height, int width) {
    PGMImage img;
    img.width = width;
    img.height = height;
    img.max_val = 255;
    img.data = allocateMatrix(height, width);

    int mx = 0;
    for (int i = 0; i < height; i++)
        for (int j = 0; j < width; j++)
            if (distance[i][j] > mx) mx = distance[i][j];

    if (mx == 0) mx = 1;

    for (int i = 0; i < height; i++)
        for (int j = 0; j < width; j++)
            img.data[i][j] =
                (unsigned char)(255.0 * distance[i][j] / mx);

    return img;
}

/**
 * @brief Convertit une matrice de bissectrice en image PGM normalisée
 */
PGMImage bisectorToPGMImage(double **matrix, int height, int width) {
    PGMImage img;
    img.width = width;
    img.height = height;
    img.max_val = 255;
    img.data = allocateMatrix(height, width);

    double mx = 0;
    for (int i = 0; i < height; i++)
        for (int j = 0; j < width; j++)
            if (matrix[i][j] > mx) mx = matrix[i][j];

    if (mx == 0) mx = 1;

    for (int i = 0; i < height; i++)
        for (int j = 0; j < width; j++)
            img.data[i][j] =
                (unsigned char)(255.0 * matrix[i][j] / mx);

    return img;
}
