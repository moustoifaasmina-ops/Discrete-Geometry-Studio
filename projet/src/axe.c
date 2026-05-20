/**
 * @file axe.c
 * @brief Implémentation de l'axe médian basé sur la fonction de bissectrice
 *        et les masques directionnels (Couprie & Zrour).
 *
 * Ce module calcule l'axe médian d'une image à partir de la carte de distance
 * euclidienne au carré (DT²).
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "axe.h"
#include "pgm.h"
#include "distanceMap.h"

#define INF (INT_MAX / 2)

/* ============================================================
 * Utilitaire local
 * ============================================================ */
static int imin(int a, int b) { return a < b ? a : b; }

/* ============================================================
 * Gestion dynamique du masque
 * ============================================================ */

/**
 * @brief Initialise un masque vide.
 */
void initMaskG(MaskG *M) {
    M->vg = NULL;
    M->ng = 0;
    M->capacity = 0;
}

/**
 * @brief Libère la mémoire du masque.
 */
void freeMaskG(MaskG *M) {
    free(M->vg);
    M->vg = NULL;
    M->ng = 0;
    M->capacity = 0;
}

/**
 * @brief Ajoute un vecteur (x,y,r) dans le masque dynamique.
 *
 * @return 1 si succès, 0 sinon (limite mémoire ou MAXWEIGHTING atteint)
 */
int AddWeighting(MaskG *M, int x, int y, int r) {
    if (M->ng >= MAXWEIGHTING) return 0;

    if (M->ng >= M->capacity) {
        int new_cap = (M->capacity == 0) ? 16 : M->capacity * 2;
        if (new_cap > MAXWEIGHTING) new_cap = MAXWEIGHTING;

        Weighting *nv = realloc(M->vg, new_cap * sizeof(Weighting));
        if (!nv) return 0;

        M->vg = nv;
        M->capacity = new_cap;
    }

    M->vg[M->ng].x = x;
    M->vg[M->ng].y = y;
    M->vg[M->ng].r = r;
    M->ng++;

    return 1;
}

/* ============================================================
 * Construction de la table CTg
 * ============================================================ */

/**
 * @brief Construit la table CTg contenant x² + y² pour chaque pixel.
 *
 * Cette table est utilisée pour la génération des masques directionnels.
 */
void CompCTg(int height, int width, int **CTg) {
    for (int y = 0; y < height; y++)
        for (int x = 0; x < width; x++)
            CTg[y][x] = x * x + y * y;
}

/* ============================================================
 * Test d'appartenance à l'axe médian
 * ============================================================ */

/**
 * @brief Teste si un pixel appartient à l'axe médian.
 *
 * @param x,y position du pixel
 * @param MgL masque directionnel
 * @param Lut table de lookup des contraintes géométriques
 * @param DTg carte de distance euclidienne au carré
 */
int IsMAg(int x, int y, MaskG *MgL, LookUpTable Lut,
          int **DTg, int height, int width) {

    int val = DTg[y][x];
    if (val == 0 || val >= MAXLUTENTRY) return 0;

    for (int i = 0; i < MgL->ng; i++) {
        int mx = MgL->vg[i].x;
        int my = MgL->vg[i].y;
        int threshold = Lut[i][val];

        if (threshold == 0) continue;

        /* symétries géométriques du vecteur */
        int syms[8][2];
        int nsym = 0;

        syms[nsym][0] =  mx; syms[nsym][1] =  my; nsym++;
        syms[nsym][0] = -mx; syms[nsym][1] =  my; nsym++;
        if (my != 0) {
            syms[nsym][0] =  mx; syms[nsym][1] = -my; nsym++;
            syms[nsym][0] = -mx; syms[nsym][1] = -my; nsym++;
        }

        if (mx != my) {
            syms[nsym][0] =  my; syms[nsym][1] =  mx; nsym++;
            syms[nsym][0] = -my; syms[nsym][1] =  mx; nsym++;
            if (mx != 0) {
                syms[nsym][0] =  my; syms[nsym][1] = -mx; nsym++;
                syms[nsym][0] = -my; syms[nsym][1] = -mx; nsym++;
            }
        }

        /* vérification des contraintes locales */
        for (int s = 0; s < nsym; s++) {
            int xx = x - syms[s][0];
            int yy = y - syms[s][1];

            if (xx >= 0 && xx < width && yy >= 0 && yy < height) {
                if (DTg[yy][xx] >= threshold)
                    return 0;
            }
        }
    }

    return 1;
}

/* ============================================================
 * Rayon maximal exploitable
 * ============================================================ */
int GreatestRadius(int size) {
    int res = (size - 1) * (size - 1) - 1;
    if (res >= MAXLUTENTRY) res = MAXLUTENTRY - 1;
    return res;
}

/* ============================================================
 * Construction de la LUT des masques
 * ============================================================ */

/**
 * @brief Calcule une ligne de la LUT pour un vecteur du masque.
 *
 * Cette LUT encode les contraintes géométriques des boules discrètes.
 */
static void computeLutRow(int **CTg, int height, int width,
                           MaskG *MgL, LookUpTable Lut,
                           int idx, int Rtarget) {

    int mx = MgL->vg[idx].x;
    int my = MgL->vg[idx].y;

    for (int r = 0; r <= Rtarget; r++) Lut[idx][r] = 0;

    for (int x = 0; x < width - mx; x++) {
        for (int y = 0; y < height && y <= x; y++) {
            if (y + my >= height) continue;

            int r1 = CTg[y][x] + 1;
            int r2 = CTg[y + my][x + mx] + 1;

            if (r1 <= Rtarget && r2 > Lut[idx][r1])
                Lut[idx][r1] = r2;
        }
    }

    /* propagation monotone */
    for (int r = 1; r <= Rtarget; r++) {
        if (Lut[idx][r] < Lut[idx][r - 1])
            Lut[idx][r] = Lut[idx][r - 1];
    }
}

/* ============================================================
 * Construction complète de la LUT du masque
 * ============================================================ */
void CompLutMask(int **CTg, int **DTg, int height, int width,
                 MaskG *MgL, LookUpTable Lut, int Rknown, int Rtarget) {

    (void)DTg;

    int Possible[MAXLUTENTRY] = {0};

    /* identification des valeurs possibles */
    for (int x = 1; x < width; x++)
        for (int y = 0; y < height && y <= x; y++) {
            int val = CTg[y][x];
            if (val < MAXLUTENTRY) Possible[val] = 1;
        }

    /* calcul LUT initiale */
    for (int i = 0; i < MgL->ng; i++)
        computeLutRow(CTg, height, width, MgL, Lut, i, Rtarget);

    /* découverte de nouveaux vecteurs */
    int full = 0;
    for (int R = Rknown + 1; R <= Rtarget && !full; R++) {
        if (!Possible[R]) continue;

        for (int x = 1; x < width && !full; x++) {
            for (int y = 0; y < height && y <= x && !full; y++) {

                if (CTg[y][x] == 0 || CTg[y][x] >= MAXLUTENTRY)
                    continue;

                if (IsMAg(x, y, MgL, Lut, CTg, height, width)) {
                    if (!AddWeighting(MgL, x, y, R)) {
                        fprintf(stderr, "[AXE] MAXWEIGHTING atteint\n");
                        full = 1;
                    } else {
                        computeLutRow(CTg, height, width, MgL, Lut,
                                      MgL->ng - 1, Rtarget);
                    }
                }
            }
        }
    }
}

/* ============================================================
 * Pipeline principal
 * ============================================================ */
static void computeMA(int **DTg, int height, int width,
                       void *output, int output_is_uchar) {

    MaskG MgL;
    initMaskG(&MgL);

    LookUpTable Lut = malloc(MAXWEIGHTING * sizeof(int *));
    for (int i = 0; i < MAXWEIGHTING; i++)
        Lut[i] = calloc(MAXLUTENTRY, sizeof(int));

    int **CTg = allocateIntMatrix(height, width);
    CompCTg(height, width, CTg);

    int Rt = GreatestRadius(imin(height, width));
    CompLutMask(CTg, DTg, height, width, &MgL, Lut, 0, Rt);

    fprintf(stderr, "[AXE] Masque : %d vecteurs\n", MgL.ng);

    /* génération sortie */
    if (output_is_uchar) {
        unsigned char **out = output;
        for (int y = 0; y < height; y++)
            for (int x = 0; x < width; x++)
                out[y][x] = IsMAg(x, y, &MgL, Lut, DTg, height, width)
                            ? 255 : 0;
    } else {
        int **out = output;
        for (int y = 0; y < height; y++)
            for (int x = 0; x < width; x++)
                out[y][x] = IsMAg(x, y, &MgL, Lut, DTg, height, width)
                            ? 255 : 0;
    }

    freeIntMatrix(CTg, height);
    for (int i = 0; i < MAXWEIGHTING; i++) free(Lut[i]);
    free(Lut);
    freeMaskG(&MgL);
}

/* ============================================================
 * API publique
 * ============================================================ */
PGMImage ComputeMedialAxisFromDT(int **DTg, int height, int width) {
    PGMImage ma;
    ma.width = width;
    ma.height = height;
    ma.max_val = 255;
    ma.data = allocateUCharMatrix(height, width);
    computeMA(DTg, height, width, ma.data, 1);
    return ma;
}

void computeMedialAxisToMatrix(int **DTg, int **medial,
                               int height, int width) {
    computeMA(DTg, height, width, medial, 0);
}
