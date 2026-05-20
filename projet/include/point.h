/**
 * @file point.h
 * @brief Module de gestion des points et ensembles de points.
 *
 * Ce module fournit une structure simple pour représenter un point 2D
 * ainsi qu’un ensemble dynamique de points (PointSet).
 */

#ifndef POINT_H
#define POINT_H

/**
 * @struct Point
 * @brief Représente un point dans un espace 2D.
 */
typedef struct {
    int x; /**< Coordonnée X */
    int y; /**< Coordonnée Y */
} Point;

/**
 * @def INITIAL_CAPACITY
 * @brief Capacité initiale d’un ensemble de points.
 */
#define INITIAL_CAPACITY 16

/**
 * @struct PointSet
 * @brief Ensemble dynamique de points.
 *
 * Cette structure permet de stocker un nombre variable de points
 * avec redimensionnement automatique.
 */
typedef struct {
    Point *points;   /**< Tableau dynamique de points */
    int size;        /**< Nombre de points actuellement stockés */
    int capacity;    /**< Capacité allouée */
} PointSet;

/* ========================= */
/* Gestion mémoire          */
/* ========================= */

/**
 * @brief Initialise un ensemble de points.
 *
 * @param ps Pointeur vers le PointSet à initialiser.
 */
void initPointSet(PointSet *ps);

/**
 * @brief Libère la mémoire d’un ensemble de points.
 *
 * @param ps Pointeur vers le PointSet à libérer.
 */
void freePointSet(PointSet *ps);

/**
 * @brief Ajoute un point dans un ensemble dynamique.
 *
 * Si nécessaire, la capacité est augmentée automatiquement.
 *
 * @param ps Pointeur vers le PointSet.
 * @param x Coordonnée X du point.
 * @param y Coordonnée Y du point.
 */
void addPoint(PointSet *ps, int x, int y);

#endif
