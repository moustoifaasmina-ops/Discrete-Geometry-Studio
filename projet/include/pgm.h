/**
 * @file pgm.h
 * @brief Module de gestion des images PGM (Portable Gray Map).
 *
 * Ce module permet de :
 * - lire et écrire des images PGM,
 * - allouer et libérer des matrices d'image,
 * - binariser une image,
 * - convertir des matrices de distance ou de bissectrice en images PGM.
 */

#ifndef PGM_H
#define PGM_H

/**
 * @struct PGMImage
 * @brief Représentation d'une image PGM en mémoire.
 */
typedef struct {
    int width;              /**< Largeur de l'image */
    int height;             /**< Hauteur de l'image */
    int max_val;            /**< Valeur maximale des pixels */
    unsigned char **data;   /**< Matrice des pixels */
} PGMImage;

/* ========================= */
/* Lecture / écriture       */
/* ========================= */

/**
 * @brief Lit une image PGM depuis un fichier.
 *
 * @param filename Chemin du fichier PGM.
 * @return Image PGM chargée en mémoire.
 */
PGMImage readPGM(const char *filename);

/**
 * @brief Écrit une image PGM dans un fichier.
 *
 * @param filename Nom du fichier de sortie.
 * @param img Image à sauvegarder.
 */
void writePGM(const char *filename, PGMImage img);

/**
 * @brief Libère la mémoire d'une image PGM.
 *
 * @param img Image à libérer.
 */
void freePGM(PGMImage img);

/* ========================= */
/* Traitement d'image       */
/* ========================= */

/**
 * @brief Binarise une image PGM selon un seuil.
 *
 * @param img Image à binariser.
 * @param threshold Seuil de binarisation.
 */
void binarizeImage(PGMImage img, unsigned char threshold);

/* ========================= */
/* Allocation mémoire       */
/* ========================= */

/**
 * @brief Alloue une matrice 2D de pixels.
 *
 * @param height Hauteur.
 * @param width Largeur.
 * @return Matrice allouée.
 */
unsigned char **allocateMatrix(int height, int width);

/**
 * @brief Libère une matrice 2D.
 *
 * @param matrix Matrice à libérer.
 * @param height Hauteur.
 */
void freeMatrix(unsigned char **matrix, int height);

/* ========================= */
/* Conversion résultats     */
/* ========================= */

/**
 * @brief Convertit une matrice de distances en image PGM.
 *
 * @param distance Matrice de distances.
 * @param height Hauteur.
 * @param width Largeur.
 * @return Image PGM résultante.
 */
PGMImage distanceToPGMImage(int **distance, int height, int width);

/**
 * @brief Convertit une matrice de bissectrice en image PGM.
 *
 * @param matrix Matrice de valeurs de bissectrice.
 * @param height Hauteur.
 * @param width Largeur.
 * @return Image PGM résultante.
 */
PGMImage bisectorToPGMImage(double **matrix, int height, int width);

#endif
