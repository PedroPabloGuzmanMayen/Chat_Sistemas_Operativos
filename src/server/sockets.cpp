#include "sockets.h"
#include "handlers.h"
#include <iostream>
#include <regex>

// Expresión regular para validar el nombre de usuario
std::regex validUsernameRegex("^[A-Za-z0-9_-]{3,16}$");

static int websocketCallback(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len) {
    switch (reason) {
        case LWS_CALLBACK_ESTABLISHED:
            std::cout << "Nueva conexión establecida.\n";
            break;

        case LWS_CALLBACK_RECEIVE: {
            if (len < 2) break;
            uint8_t messageType = ((uint8_t *)in)[0];
            uint8_t dataSize = ((uint8_t *)in)[1];
            std::vector<uint8_t> data((uint8_t *)in + 2, (uint8_t *)in + 2 + dataSize);

            // Validar nombre de usuario al recibir un mensaje de tipo 1 (registro)
            if (messageType == 1) {
                std::string username(data.begin(), data.end());
                if (username == "~" || username.empty() || !std::regex_match(username, validUsernameRegex)) {
                    std::cout << "Error: Nombre de usuario inválido.\n";
                    uint8_t errorCode = 1;  // Código de error por nombre inválido
                    sendBinaryMessage(wsi, 50, {errorCode});
                    return 1; // Cerrar la conexión
                }
            }

            handleReceivedMessage(wsi, messageType, data);
            break;
        }

        case LWS_CALLBACK_CLOSED:
            handleClientDisconnection(wsi);
            break;

        default:
            break;
    }
    return 0;
}

// Configuración del servidor WebSocket
static struct lws_protocols protocols[] = {
    { "chat-protocol", websocketCallback, 0, BUFFER_SIZE },
    { NULL, NULL, 0, 0 }
};

/**
 * Inicia el servidor WebSockets en el puerto especificado.
 */
void startServer(int port) {
    struct lws_context_creation_info info = {};
    info.port = port;
    info.protocols = protocols;
    info.gid = -1;
    info.uid = -1;

    struct lws_context *context = lws_create_context(&info);
    if (!context) {
        std::cerr << "Error al crear el contexto de WebSockets.\n";
        return;
    }

    std::cout << "Servidor WebSockets escuchando en el puerto " << port << "\n";
    while (true) {
        lws_service(context, 50);
    }

    lws_context_destroy(context);
}