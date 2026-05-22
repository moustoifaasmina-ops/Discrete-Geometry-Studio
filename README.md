# Discrete Geometry Studio

> Implémentation en C d'algorithmes de géométrie discrète pour l'analyse de formes : transformée euclidienne, axe médian et fonction bissectrice.

**Projet individuel de fin d'année**  L3 Informatique

Ce projet a été réalisé dans un cadre simulant les conditions d'une entreprise : poste de travail dédié, horaires fixes, suivi régulier avec l'encadrant, rapports d'avancement et comptes rendus à chaque étape. Les algorithmes implémentés sont issus des travaux de recherche de **Saito & Toriwaki (1994)** et **Couprie & Zrour (2005)**. Je ne suis pas l'auteure de ces algorithmes : mon travail a consisté à lire et comprendre ces articles scientifiques, puis à les implémenter intégralement en C en appliquant mes compétences en génie logiciel,
conception modulaire, gestion mémoire, pipeline de traitement, traitement de données et validation des résultats. Pour comprendre et traduire ces algorithmes en code, j'ai également dû mobiliser mes compétences en mathématiques (géométrie, algèbre, optimisation). Ce type de projet, où l'on développe des logiciels à forte composante mathématique et traitement de données, est ce qui m'intéresse le plus.

---

## Ce que fait ce projet

Ce programme prend une image en niveaux de gris (formats PGM/PPM) et extrait son **squelette géométrique** à travers un pipeline de 6 étapes :

```
Image d'entrée → Binarisation → Carte de distance → Bissectrice → Axe médian → Filtrage → Superposition
```

Concrètement, il permet de réduire une forme complexe (ex : un réseau de vaisseaux sanguins) à sa structure essentielle, tout en conservant sa topologie.

**Exemple d'utilisation** : à partir d'une image de vaisseaux sanguins, le programme extrait automatiquement leur squelette central, ce qui permet d'analyser leur épaisseur, leurs bifurcations ou de détecter des anomalies.

---

## Démonstration

### Vidéo du pipeline en action
[Capture vidéo du 20-05-2026 15:44:42.webm](https://github.com/user-attachments/assets/0704210f-3abc-4d88-ba39-7ed2aacca2c9)


### Résultat : squelette extrait

<img width="814" height="538" alt="image" src="https://github.com/user-attachments/assets/edf26e81-ce78-4bdd-836e-330843ec3203" />

---

## Aperçu du pipeline

| Étape | Sortie | Description |
|-------|--------|-------------|
| 1 | `01_binary.pgm` | Seuillage de l'image d'entrée |
| 2 | `02_edt.pgm` | Carte de distance euclidienne exacte |
| 3 | `03_bisector.pgm` | Fonction bissectrice (angle maximal) |
| 4 | `04_medial_axis.pgm` | Axe médian par boules maximales |
| 5 | `05_filtered_axis.pgm` | Axe médian filtré (suppression du bruit) |
| 6 | `06_overlay.pgm` | Superposition sur l'image d'origine |

---

## Compétences mises en œuvre

- **Autonomie complète** : projet mené seule de bout en bout, de la compréhension des articles à la livraison du logiciel
- **Langage C** : gestion manuelle de la mémoire, pointeurs, allocation dynamique, aucune dépendance externe
- **Génie logiciel** : traduction d'algorithmes théoriques (décrits dans des articles) en code structuré et fonctionnel
- **Traitement de données** : lecture, transformation et manipulation de matrices de pixels à chaque étape du pipeline, gestion des formats d'images (PGM/PPM), écriture des résultats
- **Conception logicielle** : architecture modulaire avec séparation headers / sources / données, pipeline configurable
- **Lecture scientifique** : compréhension de deux articles de recherche en anglais, extraction des équations et pseudocodes nécessaires à l'implémentation
- **Outils de développement** : Makefile, Git, compilation GCC avec flags d'optimisation
- **Tests et validation** : vérification des résultats par comparaison avec les figures et propriétés théoriques des articles originaux

---

## Structure du projet

<img width="661" height="866" alt="Capture d’écran du 2026-05-22 02-27-27" src="https://github.com/user-attachments/assets/676c48e1-493d-41ec-9a89-cd7c3469fa20" />



---

## Installation et utilisation

### Prérequis

- Linux (ou WSL)
- GCC et Make
- Doxygen (optionnel, pour la documentation)

### Compiler

```bash
git clone https://github.com/moustoifaasmina-ops/Discrete-Geometry-Studio.git
cd Discrete-Geometry-Studio
make all
```

Cela compile le programme principal (`geometrie`) et le serveur web (`server`).

### Exécuter le pipeline

```bash
./geometrie images/test_cercle.pgm output 128 0.7
```

**Paramètres** :
- `seuil_bin` (défaut 128) : seuil de binarisation
- `seuil_bisect` (défaut 0.7) : seuil de filtrage de l'axe médian (plus il est élevé, plus le squelette est simplifié)

### Lancer le serveur web

```bash
make serve
```

Le serveur démarre sur `http://localhost:8080` et permet de visualiser les résultats dans le navigateur.

### Commandes rapides

| Commande | Description |
|----------|-------------|
| `make all` | Compile le projet (programme + serveur) |
| `make run` | Exécute le pipeline sur l'image de test |
| `make serve` | Lance le serveur web de visualisation |
| `make doc` | Génère la documentation Doxygen |
| `make clean` | Nettoie tous les fichiers compilés |

---

## Algorithmes implémentés

J'ai étudié deux articles de recherche et j'ai récupéré les algorithmes qui m'étaient utiles pour construire mon pipeline de traitement d'images :

### Transformée euclidienne exacte (EDT)

Calcule pour chaque pixel sa distance exacte au bord de la forme.

- **Article d'origine** : Saito & Toriwaki, *"New algorithms for Euclidean distance transformation"*, Pattern Recognition, 1994
- **Ce que j'ai récupéré et implémenté** : l'Algorithme 4 (version optimisée) avec balayages séquentiels en 2 passes
- **Complexité** : O(n²) pour une image n×n
- **Mon fichier** : `src/distanceMap.c`

### Axe médian et fonction bissectrice

Extrait le squelette central de la forme, puis le filtre pour supprimer les branches parasites.

- **Article d'origine** : Couprie & Zrour, *"Discrete bisector function and Euclidean skeleton"*, Image and Vision Computing, 2005
- **Ce que j'ai récupéré et implémenté** : boules maximales, Extended Downstream, angle bissecteur et filtrage
- **Mes fichiers** : `src/axe.c`, `src/bisector.c`, `src/lut.c`

---

## Performances

Mesures sur un Intel i7-1185G7, GCC 11 avec -O2, image 512×512 :

| Étape | Temps |
|-------|-------|
| Binarisation | 5 ms |
| EDT | 45 ms |
| Bissectrice | 110 ms |
| Axe médian | 170 ms |
| Filtrage | 20 ms |
| **Total** | **~350 ms** |

L'EDT implémentée est **exacte** (contrairement aux approximations type Chamfer utilisées dans OpenCV) tout en restant performante grâce à l'optimisation mémoire (2 tableaux seulement).

---

## Domaines d'application

Ces algorithmes sont utilisés dans plusieurs domaines. Voici quelques exemples concrets :

### Imagerie médicale

À partir d'une image de vaisseaux sanguins, le programme peut mesurer leur épaisseur, extraire leur structure centrale et repérer d'éventuelles anomalies (rétrécissements, bifurcations). Le filtrage permet de nettoyer le résultat du bruit présent dans les images médicales.

### Vision par ordinateur et robotique

La carte de distance permet de savoir à quelle distance se trouvent les obstacles, ce qui est utile pour la navigation de robots. L'axe médian simplifie la forme d'un objet pour le reconnaître plus facilement, par exemple dans le contrôle qualité en industrie.

### Cartographie

À partir d'images satellites, on peut extraire les réseaux de rivières ou de routes, et filtrer les détails pour ne garder que les éléments principaux. C'est utile pour la prévention des inondations ou la gestion des ressources en eau.

### Reconnaissance de caractères (OCR)

Le programme peut extraire le squelette des lettres dans un document scanné, ce qui aide à mieux les reconnaître. Le filtrage supprime les taches et le bruit pour obtenir un résultat plus propre.

---

## Interface de visualisation

L'interface web actuelle a été générée avec l'assistance d'une IA pour visualiser rapidement les résultats du pipeline. Elle reste basique et sera remplacée à terme par une interface graphique en Python (Tkinter ou PyQt) que je développerai moi-même, plus complète et interactive, permettant de charger une image, ajuster les paramètres en temps réel et comparer les étapes côte à côte.

---

## Axes d'amélioration

- Développer l'interface graphique Python pour remplacer l'interface web actuelle
- Parallélisation avec OpenMP pour accélérer le traitement
- Extension à la 3D (les algorithmes le permettent avec une passe supplémentaire)
- Support GPU via CUDA pour le traitement d'images volumineuses

---

## Références

- Saito, T. & Toriwaki, J. (1994). *New algorithms for Euclidean distance transformation of an n-dimensional digitized picture with applications.* Pattern Recognition, 27(11), 1551–1565.
- Couprie, M. & Zrour, R. (2005). *Discrete bisector function and Euclidean skeleton.* Image and Vision Computing, 23(1), 81–93. [HAL](https://hal.archives-ouvertes.fr/hal-00136510/)

---

## Licence

Ce projet est un travail académique réalisé dans le cadre de ma L3 Informatique. Les algorithmes implémentés appartiennent à leurs auteurs respectifs (Saito & Toriwaki, Couprie & Zrour). L'implémentation en C est mon travail personnel.
