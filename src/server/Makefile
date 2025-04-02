CC = g++
CFLAGS = -std=c++17 -Wall -Wextra -g
LDFLAGS = -lwebsockets

# Nombre del ejecutable final
TARGET = websocket_server

# Archivos fuente
SOURCES = server.cpp DataSource.cpp logger.cpp

# Convertir archivos fuente a archivos objeto
OBJECTS = $(SOURCES:.cpp=.o)

# Regla principal que genera el ejecutable
all: $(TARGET)

# Vincular archivos objeto para crear el ejecutable
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

# Regla patrón para compilar archivos fuente a archivos objeto
%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

# Limpiar archivos generados durante la compilación
clean:
	rm -f $(OBJECTS) $(TARGET)

# Regla para recompilar todo desde cero
rebuild: clean all

# Regla para ejecutar el programa
run: all
	./$(TARGET)

# Regla para mostrar ayuda
help:
	@echo "Makefile para el servidor WebSocket"
	@echo "Comandos disponibles:"
	@echo "  make          - Compila el proyecto"
	@echo "  make clean    - Elimina archivos objeto y ejecutable"
	@echo "  make rebuild  - Recompila todo desde cero"
	@echo "  make run      - Compila y ejecuta el servidor"
	@echo "  make help     - Muestra esta ayuda"

.PHONY: all clean rebuild run help
