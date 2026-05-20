/**
 * @file ppm.h
 * @brief Module de gestion des images PPM (Portable Pixmap).
 *
 * Ce module permet de :
 * - lire et écrire des images couleur PPM,
 * - allouer et libérer des matrices 3D RGB,
 * - convertir des résultats (EDT, bissectrice) en images couleur.
 */

#ifndef PPM_H
#define PPM_H

/**
 * @struct PPMImage
 * @brief Représentation d'une image PPM en mémoire.
 *
 * Une image PPM est stockée sous forme d’une matrice 3D :
 * - dimension 1 : hauteur
 * - dimension 2 : largeur
 * - dimension 3 : canaux RGB
 */
typedef struct {
    int width;               /**< Largeur de l'image */
    int height;              /**< Hauteur de l'image */
    int max_val;             /**< Valeur maximale des pixels */
    unsigned char ***data;   /**< Matrice RGB [h][w][3] */
} PPMImage;

/* ========================= */
/* Lecture / écriture       */
/* ========================= */

/**
 * @brief Lit une image PPM depuis un fichier.
 *
 * @param filename Chemin du fichier PPM.
 * @return Image PPM chargée en mémoire.
 */
PPMImage readPPM(const char *filename);

/**
 * @brief Écrit une image PPM dans un fichier.
 *
 * @param filename Nom du fichier de sortie.
 * @param img Image à sauvegarder.
 */
void writePPM(const char *filename, PPMImage img);

/**
 * @brief Libère la mémoire d’une image PPM.
 *
 * @param img Image à libérer.
 */
void freePPM(PPMImage img);

/* ========================= */
/* Allocation mémoire       */
/* ========================= */

/**
 * @brief Alloue une matrice 3D pour image PPM.
 *
 * @param height Hauteur.
 * @param width Largeur.
 * @return Matrice RGB allouée.
 */
unsigned char ***allocateMatrixPPM(int height, int width);

/**
 * @brief Libère une matrice PPM.
 *
 * @param matrix Matrice RGB.
 * @param height Hauteur.
 * @param width Largeur.
 */
void freeMatrixPPM(unsigned char ***matrix, int height, int width);

/* ========================= */
/* Conversion résultats     */
/* ========================= */

/**
 * @brief Convertit une matrice de distances en image PPM.
 *
 * @param distance Matrice de distances (EDT).
 * @param height Hauteur.
 * @param width Largeur.
 * @return Image couleur correspondante.
 */
PPMImage distanceToPPMImage(int **distance, int height, int width);

/**
 * @brief Convertit une matrice de bissectrice en image PPM.
 *
 * @param matrix Matrice de valeurs de bissectrice.
 * @param height Hauteur.
 * @param width Largeur.
 * @return Image couleur correspondante.
 */
PPMImage bisectorToPPMImage(double **matrix, int height, int width);

#endif
