/**
 * @file distancemap.h
 * @brief Module de calcul de la transformation de distance euclidienne (EDT).
 *
 * Ce module fournit les fonctions nécessaires pour :
 * - l'allocation et la libération de matrices,
 * - la conversion d'une image binaire en carte de distance,
 * - le calcul de la transformation de distance euclidienne (EDT),
 * - l'extension avec stockage des sites (pour axe médian).
 */

#ifndef DISTANCEMAP_H
#define DISTANCEMAP_H

#include "pgm.h"
#include "point.h"

/* ========================= */
/* Allocation mémoire        */
/* ========================= */

/**
 * @brief Alloue une matrice 2D de type unsigned char.
 *
 * @param height Hauteur de la matrice.
 * @param width Largeur de la matrice.
 * @return Matrice allouée.
 */
unsigned char **allocateUCharMatrix(int height, int width);

/**
 * @brief Alloue une matrice 2D d'entiers.
 *
 * @param height Hauteur de la matrice.
 * @param width Largeur de la matrice.
 * @return Matrice allouée.
 */
int **allocateIntMatrix(int height, int width);

/**
 * @brief Libère une matrice d'entiers.
 *
 * @param matrix Matrice à libérer.
 * @param height Hauteur de la matrice.
 */
void freeIntMatrix(int **matrix, int height);

/* ========================= */
/* Initialisation            */
/* ========================= */

/**
 * @brief Initialise la carte de distance à partir d'une image binaire.
 *
 * Les pixels de fond sont initialisés à 0, les objets à valeurs infinies
 * ou maximales selon l'algorithme EDT.
 *
 * @param binary Image binaire d'entrée.
 * @param dist Matrice de distance à initialiser.
 * @param height Hauteur de l'image.
 * @param width Largeur de l'image.
 */
void initializeDistance(unsigned char **binary,
                        int **dist,
                        int height,
                        int width);

/* ========================= */
/* Transformation EDT        */
/* ========================= */

/**
 * @brief Calcule la transformation de distance euclidienne 2D.
 *
 * Algorithme classique en deux passes (horizontal + vertical)
 * basé sur Saito & Toriwaki.
 *
 * @param image Image binaire en entrée.
 * @param rows Nombre de lignes.
 * @param cols Nombre de colonnes.
 */
void euclideanDistanceTransform2D(int **image,
                                  int rows,
                                  int cols);

/**
 * @brief Calcul de l'EDT avec stockage des sites.
 *
 * Variante permettant de conserver, pour chaque pixel,
 * la position du point source le plus proche.
 *
 * @param image Image binaire.
 * @param sites Matrice des points sources associés.
 * @param rows Nombre de lignes.
 * @param cols Nombre de colonnes.
 */
void euclideanDistanceTransformWithSites2D(int **image,
                                            Point **sites,
                                            int rows,
                                            int cols);

/* ========================= */
/* Interface haut niveau     */
/* ========================= */

/**
 * @brief Calcule la carte de distance à partir d'une image PGM.
 *
 * @param img Image PGM d'entrée.
 * @return Matrice de distances euclidiennes.
 */
int **computeEDTFromPGM(PGMImage img);

/**
 * @brief Sauvegarde une matrice de distance dans un fichier texte.
 *
 * @param filename Nom du fichier de sortie.
 * @param matrix Matrice à sauvegarder.
 * @param height Hauteur.
 * @param width Largeur.
 */
void writeDistanceMatrix(const char *filename,
                         int **matrix,
                         int height,
                         int width);

#endif
