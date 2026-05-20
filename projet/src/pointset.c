/**
 * @file point.c
 * @brief Gestion dynamique d’un ensemble de points (PointSet)
 *
 * Ce module fournit une structure dynamique pour stocker des points 2D
 * avec redimensionnement automatique.
 */

#include "point.h"
#include <stdio.h>
#include <stdlib.h>

/**
 * @brief Initialise un ensemble de points
 *
 * Alloue un tableau initial de capacité INITIAL_CAPACITY.
 *
 * @param ps pointeur vers PointSet
 */
void initPointSet(PointSet *ps) {
    ps->size = 0;
    ps->capacity = INITIAL_CAPACITY;
    ps->points = malloc(ps->capacity * sizeof(Point));

    if (ps->points == NULL) {
        fprintf(stderr, "Erreur allocation PointSet\n");
        ps->capacity = 0;
    }
}

/**
 * @brief Libère un PointSet
 *
 * @param ps pointeur vers PointSet
 */
void freePointSet(PointSet *ps) {
    if (ps->points) {
        free(ps->points);
        ps->points = NULL;
        ps->size = 0;
        ps->capacity = 0;
    }
}

/**
 * @brief Ajoute un point dans l’ensemble
 *
 * Redimensionne automatiquement le tableau si nécessaire.
 *
 * @param ps pointeur vers PointSet
 * @param x coordonnée x
 * @param y coordonnée y
 */
void addPoint(PointSet *ps, int x, int y) {
    if (ps->size >= ps->capacity) {
        int newCap = ps->capacity * 2;

        Point *newPts = realloc(ps->points, newCap * sizeof(Point));
        if (!newPts) {
            fprintf(stderr, "Erreur realloc PointSet\n");
            return;
        }

        ps->points = newPts;
        ps->capacity = newCap;
    }

    ps->points[ps->size].x = x;
    ps->points[ps->size].y = y;
    ps->size++;
}
