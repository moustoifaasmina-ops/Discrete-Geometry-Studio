/**
 * @file lut.c
 * @brief Construction de la LUT (Look-Up Table) pour distances euclidiennes.
 *
 * Implémente la table de Couprie & Zrour (2005, annexe) :
 * pour chaque entier i, on stocke tous les couples (x,y)
 * tels que x² + y² = i avec x ≥ y ≥ 0.
 *
 * Cette LUT est utilisée dans :
 * - axe médian
 * - bisector function
 * - transformations géométriques discrètes
 */

#include "lut.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/// @brief Table globale LUT
PointSet *LUT = NULL;

/// @brief Taille de la LUT (nombre d’entrées)
size_t LUT_size = 0;

/**
 * @brief Construit la LUT jusqu’à N.
 *
 * Chaque entrée i contient tous les couples (x,y)
 * tels que x² + y² = i.
 *
 * @param N valeur maximale de distance au carré
 */
void buildLUT(size_t N) {
    if (LUT) freeLUT();

    LUT_size = N + 1;
    LUT = malloc(LUT_size * sizeof(PointSet));

    if (!LUT) {
        fprintf(stderr, "Erreur malloc LUT\n");
        LUT_size = 0;
        return;
    }

    for (size_t i = 0; i < LUT_size; i++)
        initPointSet(&LUT[i]);

    int max_coord = (int)ceil(sqrt((double)N));

    for (int x = 0; x <= max_coord; x++) {
        for (int y = 0; y <= x; y++) {
            int d = x * x + y * y;
            if ((size_t)d <= N)
                addPoint(&LUT[d], x, y);
        }
    }

    printf("[LUT] Construit : %zu entrées pour N=%zu\n", LUT_size, N);
}

/**
 * @brief Libère la LUT globale.
 */
void freeLUT(void) {
    if (!LUT) return;

    for (size_t i = 0; i < LUT_size; i++)
        freePointSet(&LUT[i]);

    free(LUT);
    LUT = NULL;
    LUT_size = 0;
}

/**
 * @brief Écrit la LUT dans un fichier texte.
 *
 * Format :
 * i: (x,y) (x,y) ...
 *
 * @param filename fichier de sortie
 */
void writeLUTToFile(const char *filename) {
    if (!LUT) return;

    FILE *f = fopen(filename, "w");
    if (!f) return;

    for (size_t i = 0; i < LUT_size; i++) {
        if (LUT[i].size == 0) continue;

        fprintf(f, "%zu:", i);
        for (int j = 0; j < LUT[i].size; j++) {
            fprintf(f, " (%d,%d)",
                    LUT[i].points[j].x,
                    LUT[i].points[j].y);
        }
        fprintf(f, "\n");
    }

    fclose(f);
}
