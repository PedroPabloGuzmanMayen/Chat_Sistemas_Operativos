#ifndef SOCKETS_H
#define SOCKETS_H

#include <libwebsockets.h>

/**
 * Inicia el servidor WebSockets en el puerto especificado.
 */
void startServer(int port);

#endif