/**
 * @file axe.h
 * @brief Module de calcul de l'axe médian et de la fonction bissectrice.
 *
 * Ce module implémente les structures et fonctions nécessaires à :
 * - l'extraction de l'axe médian d'une image binaire,
 * - le calcul de la fonction bissectrice,
 * - l'utilisation de la carte de distance euclidienne (EDT²),
 * - l'exploitation d'une Look-Up Table des distances.
 */

#ifndef AXE_H
#define AXE_H

#include "pgm.h"

/** @def MAXLUTENTRY
 *  @brief Taille maximale de la LUT.
 */
#define MAXLUTENTRY 100000

/** @def MAXWEIGHTING
 *  @brief Taille maximale du masque de poids.
 */
#define MAXWEIGHTING 512

/**
 * @struct Weighting
 * @brief Représente un élément géométrique (masque).
 */
typedef struct {
    int x; /**< Coordonnée X */
    int y; /**< Coordonnée Y */
    int r; /**< Rayon associé */
} Weighting;

/**
 * @struct MaskG
 * @brief Structure dynamique représentant un ensemble de poids géométriques.
 */
typedef struct {
    Weighting *vg;   /**< Tableau dynamique de poids */
    int ng;          /**< Nombre d'éléments utilisés */
    int capacity;    /**< Capacité allouée */
} MaskG;

/**
 * @brief Look-Up Table des distances euclidiennes au carré.
 *
 * LUT[r] contient les couples (x,y) tels que x² + y² = r.
 */
typedef int **LookUpTable;

/* ========================= */
/* Fonctions de gestion      */
/* ========================= */

/**
 * @brief Initialise une structure MaskG.
 */
void initMaskG(MaskG *M);

/**
 * @brief Libère la mémoire d'un MaskG.
 */
void freeMaskG(MaskG *M);

/**
 * @brief Ajoute un élément dans le masque.
 * @return 1 si succès, 0 sinon.
 */
int AddWeighting(MaskG *M, int x, int y, int r);

/* ========================= */
/* Calculs auxiliaires       */
/* ========================= */

/**
 * @brief Calcule la carte auxiliaire CTg.
 */
void CompCTg(int height, int width, int **CTg);

/**
 * @brief Teste si un pixel appartient à l'axe médian.
 * @return 1 si oui, 0 sinon.
 */
int IsMAg(int x, int y, MaskG *MgL, LookUpTable Lut,
          int **DTg, int height, int width);

/**
 * @brief Calcule le rayon maximal exploitable.
 */
int GreatestRadius(int size);

/**
 * @brief Construit les masques LUT pour la bissectrice.
 */
void CompLutMask(int **CTg, int **DTg, int height, int width,
                 MaskG *MgL, LookUpTable Lut,
                 int Rknown, int Rtarget);

/* ========================= */
/* Extraction axe médian     */
/* ========================= */

/**
 * @brief Extrait l'axe médian sous forme d'image PGM.
 */
PGMImage ComputeMedialAxisFromDT(int **DTg, int height, int width);

/**
 * @brief Écrit l'axe médian dans une matrice binaire.
 */
void computeMedialAxisToMatrix(int **DTg, int **medial,
                               int height, int width);

#endif
