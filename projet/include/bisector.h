/**
 * @file bisector.h
 * @brief Module de calcul de la fonction bissectrice.
 *
 * Ce module implémente les fonctions nécessaires au calcul de la
 * fonction bissectrice discrète à partir de la carte de distance
 * euclidienne au carré (EDT²) et d'une Look-Up Table (LUT).
 */

#ifndef BISECTOR_H
#define BISECTOR_H

#include "point.h"
#include "lut.h"
#include <stdbool.h>

/**
 * @brief Calcule l'ensemble des points "extended downstream"
 * associés à un pixel (x,y).
 *
 * Ces points sont extraits à partir de la carte de distance
 * euclidienne et de la LUT.
 *
 * @param x Coordonnée X du pixel.
 * @param y Coordonnée Y du pixel.
 * @param D_squared_map Carte de distance euclidienne au carré (EDT²).
 * @param width Largeur de l'image.
 * @param height Hauteur de l'image.
 * @param lut_table Table de correspondance des distances (LUT).
 *
 * @return Ensemble de points associés (PointSet).
 */
PointSet computeExtendedDownstream(int x, int y,
                                   int **D_squared_map,
                                   int width,
                                   int height,
                                   PointSet *lut_table);

/**
 * @brief Calcule l'angle maximal entre un point et un ensemble de points.
 *
 * Utilisé dans l'évaluation de la fonction bissectrice.
 *
 * @param x Point de référence.
 * @param points Ensemble de points voisins.
 *
 * @return Angle maximal (en radians ou unité normalisée).
 */
double computeMaxAngle(Point x, PointSet *points);

/**
 * @brief Calcule la fonction bissectrice en un point donné.
 *
 * La fonction est basée sur la carte de distance euclidienne au carré
 * et la LUT associée.
 *
 * @param x Coordonnée X du pixel.
 * @param y Coordonnée Y du pixel.
 * @param D_squared_map Carte EDT².
 * @param width Largeur de l'image.
 * @param height Hauteur de l'image.
 * @param lut_table Table LUT des distances.
 *
 * @return Valeur de la fonction bissectrice.
 */
double computeBisectorFunction(int x, int y,
                               int **D_squared_map,
                               int width,
                               int height,
                               PointSet *lut_table);

#endif
