CXX        := g++
CXXFLAGS   := -std=c++17 -Wall -Wextra -Iinclude

# FLTK para cliente
CLIENT_LDFLAGS := -lfltk -lfltk_images -lfltk_forms -lfltk_gl -lwebsockets

# libwebsockets para servidor
SERVER_LDFLAGS := -lwebsockets

# Directorios
BUILD_DIR := build
OBJ_DIR   := $(BUILD_DIR)/obj

# Fuentes
CLIENT_SRCS := $(wildcard src/client/*.cpp)
SERVER_SRCS := $(wildcard src/server/*.cpp)

# Objetos
CLIENT_OBJS := $(CLIENT_SRCS:src/client/%.cpp=$(OBJ_DIR)/client_%.o)
SERVER_OBJS := $(SERVER_SRCS:src/server/%.cpp=$(OBJ_DIR)/server_%.o)

# Ejecutables
CLIENT_BIN := $(BUILD_DIR)/client
SERVER_BIN := $(BUILD_DIR)/servidor

.PHONY: all clean

all: $(CLIENT_BIN) $(SERVER_BIN)

# --- Cliente ---
$(CLIENT_BIN): $(CLIENT_OBJS)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(CLIENT_LDFLAGS)

$(OBJ_DIR)/client_%.o: src/client/%.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# --- Servidor ---
$(SERVER_BIN): $(SERVER_OBJS)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(SERVER_LDFLAGS)

$(OBJ_DIR)/server_%.o: src/server/%.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)
