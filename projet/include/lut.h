/**
 * @file lut.h
 * @brief Module de construction et gestion de la Look-Up Table (LUT) des distances euclidiennes.
 *
 * Ce module construit une table de correspondance des distances euclidiennes au carré.
 * Pour chaque valeur r, la LUT contient l’ensemble des couples (x, y) tels que :
 *
 *      x² + y² = r
 *
 * Cette structure est utilisée dans les algorithmes de distance, de bissectrice
 * et d’axe médian pour accélérer les calculs géométriques.
 */

#ifndef LUT_H
#define LUT_H

#include <stddef.h>
#include "point.h"

/**
 * @brief Table globale des ensembles de points (LUT).
 *
 * LUT[r] contient tous les couples (x, y) tels que x² + y² = r.
 */
extern PointSet *LUT;

/**
 * @brief Taille maximale de la LUT.
 */
extern size_t LUT_size;

/* ========================= */
/* Construction / destruction */
/* ========================= */

/**
 * @brief Construit la Look-Up Table des distances euclidiennes.
 *
 * Pour chaque valeur r ≤ N, calcule tous les couples (x, y)
 * tels que x² + y² = r.
 *
 * @param N Valeur maximale de distance au carré.
 */
void buildLUT(size_t N);

/**
 * @brief Libère la mémoire de la LUT.
 */
void freeLUT(void);

/**
 * @brief Écrit la LUT dans un fichier texte.
 *
 * Format :
 * r: (x,y) (x,y) ...
 *
 * @param filename Nom du fichier de sortie.
 */
void writeLUTToFile(const char *filename);

#endif
