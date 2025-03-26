CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Iinclude
LDFLAGS := -lfltk -lfltk_images -lfltk_forms -lfltk_gl # Bibliotecas de FLTK

# Directorios
SRC_DIR := src/client
BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj

# Encuentra todos los archivos fuente en src/client
SRCS := $(wildcard $(SRC_DIR)/*.cpp)
OBJS := $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

# Nombre del ejecutable
TARGET := $(BUILD_DIR)/client

# Regla principal
all: $(TARGET)

# Compilar el ejecutable
$(TARGET): $(OBJS)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

# Compilar cada archivo fuente
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Limpiar archivos compilados
clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean
