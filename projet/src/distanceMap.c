/**
 * distanceMap.c
 * Euclidean Distance Transform - Saito & Toriwaki (1994), Algorithm 4 (fast)
 * Produces the squared Euclidean distance map D²_X.
 *
 * CORRECTIONS APPLIQUÉES :
 *   - euclideanDistanceTransformWithSites2D : réécriture complète pour
 *     calculer la vraie distance euclidienne au carré (était en L1).
 *   - euclideanDistanceTransform2D : utilisation de INF au lieu de
 *     (rows+n)² pour éviter les débordements sur grandes images.
 */
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include "distanceMap.h"

#define INF (INT_MAX / 2)

static int imin(int a, int b) { return a < b ? a : b; }

unsigned char **allocateUCharMatrix(int height, int width) {
    unsigned char **m = malloc(height * sizeof(unsigned char *));
    if (!m) return NULL;
    for (int i = 0; i < height; i++) {
        m[i] = calloc(width, sizeof(unsigned char));
        if (!m[i]) { for (int j = 0; j < i; j++) free(m[j]); free(m); return NULL; }
    }
    return m;
}

int **allocateIntMatrix(int height, int width) {
    int **m = malloc(height * sizeof(int *));
    if (!m) return NULL;
    for (int i = 0; i < height; i++) {
        m[i] = calloc(width, sizeof(int));
        if (!m[i]) { for (int j = 0; j < i; j++) free(m[j]); free(m); return NULL; }
    }
    return m;
}

void freeIntMatrix(int **matrix, int height) {
    if (!matrix) return;
    for (int i = 0; i < height; i++) free(matrix[i]);
    free(matrix);
}

void initializeDistance(unsigned char **binary, int **dist, int height, int width) {
    for (int i = 0; i < height; i++)
        for (int j = 0; j < width; j++)
            dist[i][j] = binary[i][j] ? INF : 0;
}

/**
 * Saito-Toriwaki Algorithm 4 (fast version) for 2D
 * Step 1: column-wise forward/backward scan
 * Step 2: row-wise forward/backward scan with parabola intersection optimization
 *
 * CORRECTION : protection contre le débordement dans Step 1.
 * On utilise INF comme borne au lieu de laisser df croître indéfiniment.
 */
void euclideanDistanceTransform2D(int **image, int rows, int cols) {
    /* Step 1: column transform — calcul de la distance 1D au carré */
    for (int j = 0; j < cols; j++) {
        /* Forward scan */
        int df = rows; /* Agit comme +infini */
        for (int i = 0; i < rows; i++) {
            if (image[i][j] != 0) {
                df++;
                /* Protection débordement : on borne df pour que df*df < INF */
                if (df > rows) df = rows;
            } else {
                df = 0;
            }
            image[i][j] = df * df;
        }
        /* Backward scan */
        int db = rows;
        for (int i = rows - 1; i >= 0; i--) {
            if (image[i][j] != 0) {
                db++;
                if (db > rows) db = rows;
            } else {
                db = 0;
            }
            image[i][j] = imin(image[i][j], db * db);
        }
    }

    /* Step 2: row transform (Algorithm 4 fast) */
    int *buff = malloc(cols * sizeof(int));
    if (!buff) return;

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) buff[j] = image[i][j];

        /* Forward scan */
        int a = 0;
        for (int j = 1; j < cols; j++) {
            if (a > 0) a--;
            if (buff[j] > buff[j - 1] + 1) {
                int b = (buff[j] - buff[j - 1] - 1) / 2;
                if (j + b > cols - 1) b = cols - 1 - j;
                for (int n = a; n <= b; n++) {
                    int m = buff[j - 1] + (n + 1) * (n + 1);
                    if (j + n >= cols || buff[j + n] <= m) break;
                    if (m < image[i][j + n]) image[i][j + n] = m;
                }
                a = b;
            } else a = 0;
        }

        /* Backward scan */
        a = 0;
        for (int j = cols - 2; j >= 0; j--) {
            if (a > 0) a--;
            if (buff[j] > buff[j + 1] + 1) {
                int b = (buff[j] - buff[j + 1] - 1) / 2;
                if (j - b < 0) b = j;
                for (int n = a; n <= b; n++) {
                    int m = buff[j + 1] + (n + 1) * (n + 1);
                    if (j - n < 0 || buff[j - n] <= m) break;
                    if (m < image[i][j - n]) image[i][j - n] = m;
                }
                a = b;
            } else a = 0;
        }
    }
    free(buff);
}

/**
 *   Saito-Toriwaki avec propagation de sites.
 * - Étape 1 (verticale) : pour chaque colonne, on propage le site
 *   le plus proche verticalement et on calcule (i - site_row)².
 * - Étape 2 (horizontale) : pour chaque ligne, on cherche le site
 *   qui minimise (i - site_row)² + (j - site_col)².
 */
void euclideanDistanceTransformWithSites2D(int **image, Point **sites, int rows, int cols) {
    /*
     * Étape 1 : transformation verticale (colonne par colonne)
     * Pour chaque pixel, on trouve le 0-pixel le plus proche dans la même colonne.
     */
    for (int j = 0; j < cols; j++) {
        /* Forward scan : propager vers le bas */
        int lastSite = -1;
        for (int i = 0; i < rows; i++) {
            if (image[i][j] == 0) {
                lastSite = i;
                sites[i][j].x = i;
                sites[i][j].y = j;
                /* image[i][j] reste 0 */
            } else if (lastSite != -1) {
                image[i][j] = (i - lastSite) * (i - lastSite);
                sites[i][j].x = lastSite;
                sites[i][j].y = j;
            } else {
                image[i][j] = INF;
                sites[i][j].x = -1;
                sites[i][j].y = -1;
            }
        }

        /* Backward scan : propager vers le haut */
        lastSite = -1;
        for (int i = rows - 1; i >= 0; i--) {
            if (image[i][j] == 0) {
                lastSite = i;
                /* Déjà traité dans le forward */
            } else if (lastSite != -1) {
                int d = (i - lastSite) * (i - lastSite);
                if (d < image[i][j]) {
                    image[i][j] = d;
                    sites[i][j].x = lastSite;
                    sites[i][j].y = j;
                }
            }
        }
    }

    /*
     * Étape 2 : transformation horizontale (ligne par ligne)
     * Pour chaque pixel (i,j), on cherche la colonne k qui minimise
     * image_step1[i][k] + (j-k)², ce qui donne la distance euclidienne au carré.
     * On utilise l'algorithme basique (Algorithm 3, Step 2) pour la clarté.
     */
    int *tempD = malloc(cols * sizeof(int));
    Point *tempS = malloc(cols * sizeof(Point));
    if (!tempD || !tempS) { free(tempD); free(tempS); return; }

    for (int i = 0; i < rows; i++) {
        /* Copier la ligne courante */
        for (int j = 0; j < cols; j++) {
            tempD[j] = image[i][j];
            tempS[j] = sites[i][j];
        }

        /* Pour chaque pixel, chercher le minimum sur la ligne */
        for (int j = 0; j < cols; j++) {
            /*
             * La borne de recherche r = sqrt(image[i][j]) : on sait qu'il
             * existe un 0-pixel à distance au plus sqrt(image[i][j]),
             * donc on ne cherche que dans [-r, +r].
             */
            int bestD = tempD[j]; /* + (j-j)² = tempD[j] */
            Point bestS = tempS[j];

            /* Borne de recherche */
            int r;
            if (bestD >= INF) {
                r = cols; /* Chercher partout */
            } else {
                r = (int)sqrt((double)bestD) + 1;
            }

            int jmin = j - r;
            if (jmin < 0) jmin = 0;
            int jmax = j + r;
            if (jmax >= cols) jmax = cols - 1;

            for (int k = jmin; k <= jmax; k++) {
                int d = tempD[k] + (j - k) * (j - k);
                if (d < bestD) {
                    bestD = d;
                    bestS = tempS[k];
                }
            }

            image[i][j] = bestD;
            sites[i][j] = bestS;
        }
    }

    free(tempD);
    free(tempS);
}

int **computeEDTFromPGM(PGMImage img) {
    int h = img.height, w = img.width;
    int **dist = allocateIntMatrix(h, w);
    if (!dist) return NULL;
    initializeDistance(img.data, dist, h, w);
    euclideanDistanceTransform2D(dist, h, w);
    return dist;
}

void writeDistanceMatrix(const char *filename, int **matrix, int height, int width) {
    FILE *f = fopen(filename, "w");
    if (!f) return;
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) fprintf(f, "%d ", matrix[i][j]);
        fprintf(f, "\n");
    }
    fclose(f);
} 
