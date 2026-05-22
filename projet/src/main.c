/**
 * main.c — Pipeline complet de géométrie discrète
 *
 * Accepte PGM (P5) et PPM (P6) en entrée.
 * Si l'entrée est un PPM, il est converti en niveaux de gris (PGM)
 * avant le pipeline : gris = 0.299*R + 0.587*G + 0.114*B
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <time.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "pgm.h"
#include "ppm.h"
#include "distanceMap.h"
#include "lut.h"
#include "bisector.h"
#include "axe.h"

/**
 * Détecte le format d'un fichier image (P5 ou P6).
 * Retourne 5 pour PGM, 6 pour PPM, 0 si inconnu.
 */
static int detectFormat(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) return 0;
    char magic[3] = {0};
    if (fread(magic, 1, 2, f) != 2) { fclose(f); return 0; }
    fclose(f);
    if (magic[0] == 'P' && magic[1] == '5') return 5;
    if (magic[0] == 'P' && magic[1] == '6') return 6;
    return 0;
}

/**
 * Lit un PPM (P6) et le convertit en PGM (niveaux de gris).
 * Formule ITU-R BT.601 : gris = 0.299*R + 0.587*G + 0.114*B
 */
static PGMImage readPPMasPGM(const char *filename) {
    PGMImage pgm = {0, 0, 0, NULL};

    FILE *f = fopen(filename, "rb");
    if (!f) { perror("Erreur ouverture PPM"); return pgm; }

    char magic[3];
    if (fscanf(f, "%2s", magic) != 1 || strcmp(magic, "P6") != 0) {
        fprintf(stderr, "Format PPM non supporté (P6 attendu)\n");
        fclose(f); return pgm;
    }

    int width = 0, height = 0, max_val = 0;
    char line[256];
    int valuesRead = 0;

    while (valuesRead < 3) {
        if (!fgets(line, sizeof(line), f)) {
            fprintf(stderr, "Erreur lecture en-tête PPM\n");
            fclose(f); return pgm;
        }
        if (line[0] == '#') continue;
        char *ptr = line;
        while (valuesRead < 3) {
            while (*ptr && (*ptr == ' ' || *ptr == '\t' || *ptr == '\n' || *ptr == '\r')) ptr++;
            if (!*ptr || *ptr == '#') break;
            int val, n;
            if (sscanf(ptr, "%d%n", &val, &n) == 1) {
                if (valuesRead == 0) width = val;
                else if (valuesRead == 1) height = val;
                else max_val = val;
                valuesRead++;
                ptr += n;
            } else break;
        }
    }

    if (width <= 0 || height <= 0 || max_val <= 0 || max_val > 255) {
        fprintf(stderr, "En-tête PPM invalide\n");
        fclose(f); return pgm;
    }

    /* Consommer le whitespace après max_val */
    int c = fgetc(f);
    if (c != '\n' && c != '\r') ungetc(c, f);

    /* Lire les pixels RGB et convertir en gris */
    pgm.width = width;
    pgm.height = height;
    pgm.max_val = 255;
    pgm.data = allocateMatrix(height, width);
    if (!pgm.data) { fclose(f); pgm.width = 0; return pgm; }

    unsigned char rgb[3];
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (fread(rgb, 1, 3, f) != 3) {
                fprintf(stderr, "Erreur lecture pixel PPM (%d,%d)\n", y, x);
                fclose(f); freePGM(pgm);
                pgm.data = NULL; pgm.width = 0;
                return pgm;
            }
            /* Conversion en niveaux de gris */
            pgm.data[y][x] = (unsigned char)(0.299 * rgb[0] + 0.587 * rgb[1] + 0.114 * rgb[2]);
        }
    }

    fclose(f);
    return pgm;
}

static void writeOverlayPGM(const char *filename,
                             unsigned char **binary, int **medial,
                             double **bisector, double seuil,
                             int height, int width)
{
    PGMImage img;
    img.width = width; img.height = height; img.max_val = 255;
    img.data = allocateMatrix(height, width);

    for (int y = 0; y < height; y++)
        for (int x = 0; x < width; x++) {
            if (medial[y][x] && bisector[y][x] >= seuil)
                img.data[y][x] = 255;
            else if (medial[y][x])
                img.data[y][x] = 180;
            else if (binary[y][x])
                img.data[y][x] = 64;
            else
                img.data[y][x] = 0;
        }

    writePGM(filename, img);
    freePGM(img);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr,
            "Usage: %s <input.pgm|input.ppm> <output_dir> [seuil_bin=128] [seuil_bisect=0.7]\n",
            argv[0]);
        return EXIT_FAILURE;
    }

    const char *input_file = argv[1];
    const char *out_dir = argv[2];
    unsigned char seuil_bin = argc > 3 ? (unsigned char)atoi(argv[3]) : 128;
    double seuil_bisect = argc > 4 ? atof(argv[4]) : 0.7;

    char path[512];

    printf("------------------------------------------\n");
    printf("  DISCRETE GEOMETRY STUDIO\n");
    printf("------------------------------------------\n\n");

    /* 1. Lecture - détection automatique PGM / PPM */
    printf("[1/7] Lecture de %s...\n", input_file);

    int fmt = detectFormat(input_file);
    PGMImage img;

    if (fmt == 5) {
        img = readPGM(input_file);
        printf("       Format : PGM (P5)\n");
    } else if (fmt == 6) {
        img = readPPMasPGM(input_file);
        printf("       Format : PPM (P6) -> converti en niveaux de gris\n");
    } else {
        fprintf(stderr, "Erreur: format non reconnu (attendu P5 ou P6)\n");
        return EXIT_FAILURE;
    }

    if (!img.data) {
        fprintf(stderr, "Erreur: impossible de lire %s\n", input_file);
        return EXIT_FAILURE;
    }

    printf("       Dimensions : %d x %d, max_val=%d\n", img.width, img.height, img.max_val);
    int H = img.height, W = img.width;

    /* 2. Binarisation */
    printf("[2/7] Binarisation (seuil=%d)...\n", seuil_bin);
    binarizeImage(img, seuil_bin);
    snprintf(path, sizeof(path), "%s/01_binary.pgm", out_dir);
    writePGM(path, img);
    printf("       -> %s\n", path);

    /* 3. EDT */
    printf("[3/7] Calcul EDT (Saito-Toriwaki Algorithm 4)...\n");
    clock_t t0 = clock();
    int **D2 = computeEDTFromPGM(img);
    clock_t t1 = clock();
    double dt_edt = (double)(t1 - t0) / CLOCKS_PER_SEC * 1000.0;

    int maxD2 = 0;
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++)
            if (D2[y][x] > maxD2) maxD2 = D2[y][x];
    printf("       Temps EDT : %.1f ms\n", dt_edt);
    printf("       Max D² = %d (rayon max = %.2f)\n", maxD2, sqrt((double)maxD2));

    snprintf(path, sizeof(path), "%s/02_edt.pgm", out_dir);
    PGMImage edtImg = distanceToPGMImage(D2, H, W);
    writePGM(path, edtImg);
    freePGM(edtImg);
    printf("       -> %s\n", path);

    snprintf(path, sizeof(path), "%s/distance_matrix.txt", out_dir);
    writeDistanceMatrix(path, D2, H, W);

    /* 4. LUT */
    printf("[4/7] Construction LUT (décompositions en somme de carrés)...\n");
    size_t lutN = maxD2 > 0 ? (size_t)maxD2 : 1;
    buildLUT(lutN);
    snprintf(path, sizeof(path), "%s/lut.txt", out_dir);
    writeLUTToFile(path);

    /* 5. Axe médian */
    printf("[5/7] Extraction axe médian (boules maximales)...\n");
    clock_t t4 = clock();
    int **medial = allocateIntMatrix(H, W);
    computeMedialAxisToMatrix(D2, medial, H, W);
    clock_t t5 = clock();
    double dt_ma = (double)(t5 - t4) / CLOCKS_PER_SEC * 1000.0;

    int maCount = 0;
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++)
            if (medial[y][x]) maCount++;
    printf("       Temps axe médian : %.1f ms\n", dt_ma);
    printf("       Points axe médian : %d\n", maCount);

    snprintf(path, sizeof(path), "%s/04_medial_axis.pgm", out_dir);
    PGMImage maImg;
    maImg.width = W; maImg.height = H; maImg.max_val = 255;
    maImg.data = allocateMatrix(H, W);
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++)
            maImg.data[y][x] = medial[y][x] ? 255 : 0;
    writePGM(path, maImg);
    freePGM(maImg);
    printf("       -> %s\n", path);

    /* 6. Bissectrice (axe médian seulement) */
    printf("[6/7] Calcul fonction bissectrice θ_X (axe médian seulement)...\n");
    clock_t t2 = clock();

    double **bisector = malloc(H * sizeof(double *));
    for (int i = 0; i < H; i++)
        bisector[i] = calloc(W, sizeof(double));

    int bisPixels = 0;
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++)
            if (medial[y][x]) {
                bisector[y][x] = computeBisectorFunction(x, y, D2, W, H, LUT);
                bisPixels++;
            }

    clock_t t3 = clock();
    double dt_bis = (double)(t3 - t2) / CLOCKS_PER_SEC * 1000.0;

    double maxBis = 0;
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++)
            if (bisector[y][x] > maxBis) maxBis = bisector[y][x];
    printf("       Temps bissectrice : %.1f ms (%d pixels traités)\n", dt_bis, bisPixels);
    printf("       Max θ = %.4f rad (%.1f°)\n", maxBis, maxBis * 180.0 / M_PI);

    snprintf(path, sizeof(path), "%s/03_bisector.pgm", out_dir);
    PGMImage bisImg = bisectorToPGMImage(bisector, H, W);
    writePGM(path, bisImg);
    freePGM(bisImg);
    printf("       -> %s\n", path);

    /* 7. Filtrage */
    printf("[7/7] Filtrage par seuil bissectrice (θ ≥ %.2f rad = %.1f°)...\n",
           seuil_bisect, seuil_bisect * 180.0 / M_PI);

    int filtCount = 0;
    PGMImage filtImg;
    filtImg.width = W; filtImg.height = H; filtImg.max_val = 255;
    filtImg.data = allocateMatrix(H, W);
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++) {
            if (medial[y][x] && bisector[y][x] >= seuil_bisect) {
                filtImg.data[y][x] = 255;
                filtCount++;
            } else {
                filtImg.data[y][x] = 0;
            }
        }
    printf("       Points filtrés : %d / %d\n", filtCount, maCount);

    snprintf(path, sizeof(path), "%s/05_filtered_axis.pgm", out_dir);
    writePGM(path, filtImg);
    freePGM(filtImg);
    printf("       -> %s\n", path);

    snprintf(path, sizeof(path), "%s/06_overlay.pgm", out_dir);
    writeOverlayPGM(path, img.data, medial, bisector, seuil_bisect, H, W);
    printf("       -> %s\n", path);

    /* Résumé */
    printf("\n--------------------------------------\n");
    printf("  RÉSUMÉ\n");
    printf("----------------------------------------\n");
    printf("  Image         : %s (%dx%d)\n", input_file, W, H);
    printf("  Format        : %s\n", fmt == 6 ? "PPM (P6) → PGM" : "PGM (P5)");
    printf("  Seuil bin.    : %d\n", seuil_bin);
    printf("  Seuil bisect. : %.2f rad\n", seuil_bisect);
    printf("  Temps EDT     : %.1f ms\n", dt_edt);
    printf("  Temps axe méd.: %.1f ms\n", dt_ma);
    printf("  Temps bisect. : %.1f ms (%d pts)\n", dt_bis, bisPixels);
    printf("  Temps total   : %.1f ms\n", dt_edt + dt_ma + dt_bis);
    printf("  Max D²        : %d\n", maxD2);
    printf("  Max θ         : %.4f rad\n", maxBis);
    printf("  Pts axe méd.  : %d\n", maCount);
    printf("  Pts filtrés   : %d\n", filtCount);
    printf("  LUT entrées   : %zu\n", LUT_size);
    printf("  Sorties dans  : %s/\n", out_dir);
    printf("-----------------------------------------\n");

    /* Nettoyage */
    freePGM(img);
    freeIntMatrix(D2, H);
    freeIntMatrix(medial, H);
    for (int i = 0; i < H; i++) free(bisector[i]);
    free(bisector);
    freeLUT();

    return EXIT_SUCCESS;
}
