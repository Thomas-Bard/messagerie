CC=gcc
CFLAGS=-Wall -Wextra -std=c11
LDFLAGS=-lpthread -lncurses

# Répertoires
SRC_DIR=src
INCLUDE_DIR=includes
BIN_DIR=bin
OBJ_DIR=$(BIN_DIR)/obj
BUILD_DIR=$(BIN_DIR)/build

# Liste des fichiers sources
SRCS=$(wildcard $(SRC_DIR)/*.c)
# Générer la liste des fichiers objets à partir de la liste des fichiers sources
OBJS=$(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

# Remplacez le nom du fichier exécutable par celui de votre choix
TARGET=$(BUILD_DIR)/messagerie

# Règle par défaut pour construire le programme
all: directories $(TARGET)

# Règle pour construire le programme
$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

# Règle générique pour construire les fichiers objets dans le répertoire obj/
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) -c -o $@ $< $(CFLAGS) -I$(INCLUDE_DIR)

# Crée les répertoires nécessaires
directories:
	mkdir -p $(OBJ_DIR) $(BUILD_DIR)

# Règle pour nettoyer les fichiers objets et l'exécutable
clean:
	rm -rf $(BIN_DIR)
