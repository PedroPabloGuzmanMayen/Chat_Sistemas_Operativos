#include "network.h"
#include <iostream>
#include <vector>
#include <cstring>
#include <libwebsockets.h>

struct lws *webSocket = nullptr;
std::vector<std::string> userList;
std::vector<std::string> chatHistory;
std::string errorMessage;

// ✅ Definir las variables fuera del `switch`
uint8_t messageType;
uint8_t dataSize;
std::vector<uint8_t> data;

/**
 * Callback para manejar eventos de WebSockets
 */
static int websocketCallback(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len) {
    switch (reason) {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            std::cout << "Conectado al servidor.\n";
            break;

        case LWS_CALLBACK_CLIENT_RECEIVE:
            if (len < 2) break;

            // ✅ Ahora las variables ya están declaradas fuera del switch
            messageType = ((uint8_t *)in)[0];
            dataSize = ((uint8_t *)in)[1];
            data.assign((uint8_t *)in + 2, (uint8_t *)in + 2 + dataSize);

            handleServerMessage(messageType, data);
            break;

        case LWS_CALLBACK_CLIENT_CLOSED:
            std::cout << "Desconectado del servidor.\n";
            break;

        default:
            break;
    }
    return 0;
}

/**
 * Conectar al servidor WebSockets
 */
bool connectToServer(const std::string &ip, int port) {
    struct lws_context_creation_info info = {};
    info.port = CONTEXT_PORT_NO_LISTEN;

    // ✅ Definir protocols correctamente
    static struct lws_protocols protocols[] = {
        { "chat-protocol", websocketCallback, 0, BUFFER_SIZE },
        { NULL, NULL, 0, 0 }
    };

    info.protocols = protocols;
    info.gid = -1;
    info.uid = -1;

    struct lws_client_connect_info ccinfo = {};
    std::string url = "ws://" + ip + ":" + std::to_string(port) + "?name=cliente";

    ccinfo.context = lws_create_context(&info);
    ccinfo.address = ip.c_str();
    ccinfo.port = port;
    ccinfo.path = url.c_str();
    ccinfo.host = ip.c_str();
    ccinfo.origin = ip.c_str();
    ccinfo.protocol = "chat-protocol";
    ccinfo.pwsi = &webSocket;

    if (!lws_client_connect_via_info(&ccinfo)) {
        std::cerr << "Error al conectar con el servidor.\n";
        return false;
    }

    std::cout << "Conectando a " << url << "\n";
    return true;
}