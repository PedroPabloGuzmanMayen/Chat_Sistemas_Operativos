#ifndef SOCKETS_H
#define SOCKETS_H

#include <libwebsockets.h>

#define BUFFER_SIZE 1024  // ✅ Se define BUFFER_SIZE aquí

/**
 * Inicia el servidor WebSockets en el puerto especificado.
 */
void startServer(int port);

#endif