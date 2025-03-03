#include "network.h"
#include <iostream>
#include <vector>
#include <cstring>
#include <libwebsockets.h>

struct lws *webSocket = nullptr;
std::vector<std::string> userList;
std::vector<std::string> chatHistory;
std::string errorMessage;

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
            uint8_t messageType = ((uint8_t *)in)[0];
            uint8_t dataSize = ((uint8_t *)in)[1];
            std::vector<uint8_t> data((uint8_t *)in + 2, (uint8_t *)in + 2 + dataSize);

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

/**
 * Manejo de mensajes recibidos del servidor
 */
void handleServerMessage(uint8_t messageType, const std::vector<uint8_t> &data) {
    switch (messageType) {
        case 51: { // Lista de usuarios conectados
            userList.clear();
            int numUsers = data[0];
            int index = 1;
            for (int i = 0; i < numUsers; i++) {
                int nameLength = data[index++];
                std::string username(data.begin() + index, data.begin() + index + nameLength);
                index += nameLength;
                userList.push_back(username);
            }
            break;
        }

        case 55: { // Mensaje recibido
            std::string message(data.begin(), data.end());
            chatHistory.push_back(message);
            break;
        }

        case 56: { // Historial de mensajes
            chatHistory.clear();
            int numMessages = data[0];
            int index = 1;
            for (int i = 0; i < numMessages; i++) {
                int msgLength = data[index++];
                std::string msg(data.begin() + index, data.begin() + index + msgLength);
                index += msgLength;
                chatHistory.push_back(msg);
            }
            break;
        }

        case 50: { // Error recibido
            errorMessage = "Error: " + std::to_string(data[0]);
            break;
        }
    }
}

/**
 * Enviar mensaje al servidor
 */
void sendMessageToServer(uint8_t messageType, const std::vector<uint8_t> &data) {
    if (!webSocket) return;

    std::vector<uint8_t> buffer(2 + data.size());
    buffer[0] = messageType;
    buffer[1] = data.size();
    std::memcpy(buffer.data() + 2, data.data(), data.size());

    lws_write(webSocket, buffer.data(), buffer.size(), LWS_WRITE_BINARY);
}