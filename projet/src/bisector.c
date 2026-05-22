/**
 * @file bisector.c
 * @brief Fonction de bissectrice discrète (Couprie & Zrour, 2005)
 *
 * Ce module implémente la fonction de bissectrice discrète basée sur :
 *  - la transformation de distance euclidienne au carré (D²)
 *  - la décomposition en ensembles de points (LUT)
 *  - l'analyse angulaire des structures locales
 *
 * OPTIMISATIONS :
 *  - suppression de la recherche linéaire via table visited O(1)
 *  - réduction des duplications de symétries
 *  - exploitation de la symétrie des points (a >= b >= 0 dans la LUT)
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include "point.h"
#include "lut.h"
#include "bisector.h"

static bool isInBounds(int px, int py, int width, int height) {
    return px >= 0 && px < width && py >= 0 && py < height;
}

/ * Extension du downstream (coeur de la méthode)*/

/**
 * @brief Calcule l'ensemble étendu des points downstream.
 *
 * Cet ensemble correspond aux points géométriquement atteignables
 * depuis une position (x,y) en utilisant la LUT des décompositions
 * de disques discrets.
 *
 * OPTIMISATION :
 * - utilisation d'une table visited[] pour éviter les doublons O(k)
 *   remplacée par O(1)
 *
 * @param x,y point centre
 * @param D2 carte de distance euclidienne au carré
 * @param lut_table table LUT des décompositions de disques
 */
PointSet computeExtendedDownstream(int x, int y, int **D2,
                                    int width, int height,
                                    PointSet *lut_table) {

    PointSet eds;
    initPointSet(&eds);

    /* Table de marquage des points déjà ajoutés (O(1)) */
    unsigned char *visited =
        calloc((size_t)width * height, sizeof(unsigned char));

    if (!visited) return eds;

    /* voisinage 4-connecté + centre */
    int neighbors[5][2] = {
        {0, 0}, {1, 0}, {0, 1}, {-1, 0}, {0, -1}
    };

    for (int n = 0; n < 5; n++) {
        int nx = x + neighbors[n][0];
        int ny = y + neighbors[n][1];

        if (!isInBounds(nx, ny, width, height))
            continue;

        int R = D2[ny][nx];
        if (R < 0 || (size_t)R >= LUT_size)
            continue;

        PointSet *decomps = &lut_table[R];
        if (!decomps->points || decomps->size == 0)
            continue;

        /* exploration des décompositions du disque */
        for (int i = 0; i < decomps->size; i++) {

            int a = decomps->points[i].x;
            int b = decomps->points[i].y;

            /* 
             * Génération des symétries discrètes du disque
             *
             * LUT stocke uniquement (a,b) avec a >= b >= 0
             * On reconstruit toutes les directions possibles :
             *   (±a, ±b) et (±b, ±a)
             *
             * Cas particuliers :
             *  - a == b → duplication des permutations
             *  - a == 0 ou b == 0 -> réduction du nombre de symétries
             *  */
            int syms[8][2];
            int nsym = 0;

            syms[nsym][0] =  a; syms[nsym][1] =  b; nsym++;

            if (a != 0) {
                syms[nsym][0] = -a; syms[nsym][1] =  b; nsym++;
            }
            if (b != 0) {
                syms[nsym][0] =  a; syms[nsym][1] = -b; nsym++;
            }
            if (a != 0 && b != 0) {
                syms[nsym][0] = -a; syms[nsym][1] = -b; nsym++;
            }

            if (a != b) {
                syms[nsym][0] =  b; syms[nsym][1] =  a; nsym++;

                if (b != 0) {
                    syms[nsym][0] = -b; syms[nsym][1] =  a; nsym++;
                }
                if (a != 0) {
                    syms[nsym][0] =  b; syms[nsym][1] = -a; nsym++;
                }
                if (a != 0 && b != 0) {
                    syms[nsym][0] = -b; syms[nsym][1] = -a; nsym++;
                }
            }

            /* insertion des points valides */
            for (int s = 0; s < nsym; s++) {
                int zx = nx + syms[s][0];
                int zy = ny + syms[s][1];

                if (isInBounds(zx, zy, width, height) &&
                    D2[zy][zx] == 0) {

                    int idx = zy * width + zx;

                    if (!visited[idx]) {
                        visited[idx] = 1;
                        addPoint(&eds, zx, zy);
                    }
                }
            }
        }
    }

    free(visited);
    return eds;
}

/*
  Calcul de l'angle maximal
 */

/**
 * @brief Calcule l'angle maximal formé par deux points autour d'un centre.
 *
 * Utilisé pour détecter les zones de discontinuité géométrique
 * (fondement de la bissectrice discrète).
 */
double computeMaxAngle(Point center, PointSet *points) {

    if (points->size < 2)
        return 0.0;

    double maxAngle = 0.0;

    for (int i = 0; i < points->size; i++) {

        double dx1 = points->points[i].x - center.x;
        double dy1 = points->points[i].y - center.y;
        double n1 = sqrt(dx1 * dx1 + dy1 * dy1);
        if (n1 == 0.0) continue;

        for (int j = i + 1; j < points->size; j++) {

            double dx2 = points->points[j].x - center.x;
            double dy2 = points->points[j].y - center.y;
            double n2 = sqrt(dx2 * dx2 + dy2 * dy2);
            if (n2 == 0.0) continue;

            double cosA = (dx1 * dx2 + dy1 * dy2) / (n1 * n2);

            /* stabilité numérique */
            if (cosA > 1.0) cosA = 1.0;
            if (cosA < -1.0) cosA = -1.0;

            double angle = acos(cosA);

            if (angle > maxAngle)
                maxAngle = angle;
        }
    }

    return maxAngle;
}

/* 
  Fonction de bissectrice discrète
*/

/**
 * @brief Calcule la valeur de la fonction de bissectrice en (x,y).
 *
 * La fonction mesure la dispersion angulaire locale des structures
 * issues de la transformée de distance.
 *
 * @return valeur de bissectrice (angle max)
 */
double computeBisectorFunction(int x, int y, int **D2,
                                int width, int height,
                                PointSet *lut_table) {

    PointSet eds =
        computeExtendedDownstream(x, y, D2, width, height, lut_table);

    Point center = {x, y};

    double angle = computeMaxAngle(center, &eds);

    freePointSet(&eds);

    return angle;
}
