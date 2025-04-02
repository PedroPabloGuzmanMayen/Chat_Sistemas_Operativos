# Compilador y flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -pthread -g `fltk-config --cxxflags` -I../../include

# Archivos fuente
SRC = client.cpp
OBJ = $(SRC:.cpp=.o)

# Librerías necesarias
LIBS = `fltk-config --ldflags` -lboost_system -lboost_thread

# Nombre del ejecutable
TARGET = client_gui

# Regla por defecto
all: $(TARGET)

# Cómo compilar
$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

# Limpiar
clean:
	rm -f $(OBJ) $(TARGET)