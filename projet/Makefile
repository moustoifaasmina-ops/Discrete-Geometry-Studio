
# Makefile — Discrete Geometry Studio

CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -std=c99 -I./include
LDFLAGS = -lm

SRC_DIR = src
OBJ_DIR = obj
BIN     = geometrie
SERVER  = server

#  DOXYGEN CONFIG
DOC_DIR = docs
DOXYFILE = Doxyfile

SRCS = $(SRC_DIR)/main.c       \
       $(SRC_DIR)/pgm.c        \
       $(SRC_DIR)/ppm.c        \
       $(SRC_DIR)/distanceMap.c \
       $(SRC_DIR)/pointset.c   \
       $(SRC_DIR)/lut.c        \
       $(SRC_DIR)/bisector.c   \
       $(SRC_DIR)/axe.c

OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

.PHONY: all clean run serve doc doxy doc-clean

#  COMPILATION

all: $(OBJ_DIR) $(BIN) $(SERVER)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo ""
	@echo "==> Compilation réussie : ./$(BIN)"
	@echo "    Usage : ./$(BIN) <input.pgm> <output_dir> [seuil_bin] [seuil_bisect]"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(SERVER): $(SRC_DIR)/server.c
	$(CC) -Wall -Wextra -O2 -std=c99 -o $@ $< -lpthread
	@echo "==> Serveur compilé : ./$(SERVER)"

#  DOXYGEN DOCUMENTATION


doc: doxy

doxy:
	@echo "---------------------------------------------"
	@echo " Génération documentation Doxygen..."
	@echo "---------------------------------------------"
	@mkdir -p $(DOC_DIR)
	@doxygen $(DOXYFILE)
	@echo ""
	@echo " Documentation générée : $(DOC_DIR)/html/index.html"
	@echo ""

#  Ouvrir la doc directement (optionnel)
doc-open:
	xdg-open $(DOC_DIR)/html/index.html

#  Nettoyage de la doc uniquement
doc-clean:
	rm -rf $(DOC_DIR)
	@echo "Documentation supprimée."


#  EXECUTION


run: $(BIN)
	mkdir -p output
	./$(BIN) images/test_cercle.pgm output 128 0.7

serve: $(BIN) $(SERVER)
	@echo ""
	@echo "  Lancement du serveur web..."
	@echo "  Ouvrir : http://localhost:8080"
	@echo ""
	./$(SERVER)

#  CLEAN GLOBAL

clean:
	rm -rf $(OBJ_DIR) $(BIN) $(SERVER) web_output $(DOC_DIR)
	@echo "Nettoyage effectué."
