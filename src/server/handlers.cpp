#include "handlers.h"
#include "messages.h"
#include "users.h"
#include <iostream>
#include <vector>
#include <mutex>
#include <cstring>

std::mutex clients_mutex;
std::unordered_map<std::string, Client> clients;

/**
 * Envía un mensaje binario a un cliente específico
 */
void sendBinaryMessage(struct lws *wsi, uint8_t messageType, const std::vector<uint8_t> &data) {
    std::vector<uint8_t> buffer(2 + data.size());
    buffer[0] = messageType;
    buffer[1] = data.size();
    std::memcpy(buffer.data() + 2, data.data(), data.size());

    lws_write(wsi, buffer.data(), buffer.size(), LWS_WRITE_BINARY);
}

/**
 * Manejo de recepción de mensajes con validación de errores
 */
void handleReceivedMessage(struct lws *wsi, uint8_t messageType, const std::vector<uint8_t> &data) {
    if (data.empty()) {
        std::cout << "Error: Mensaje vacío.\n";
        sendBinaryMessage(wsi, 50, {3}); // Código de error 3: Mensaje vacío
        return;
    }

    switch (messageType) {
        case 1: handleRegisterUser(wsi, data); break;
        case 2: handleListUsers(wsi); break;
        case 3: handleChangeStatus(wsi, data); break;
        case 4: handleSendMessage(wsi, data); break;
        case 5: handleGetMessageHistory(wsi, data); break;
        default:
            std::cout << "Error: Tipo de mensaje desconocido.\n";
            sendBinaryMessage(wsi, 50, {1}); // Código de error 1: Mensaje inválido
            break;
    }
}

/**
 * Manejo de desconexión de clientes
 */
void handleClientDisconnection(struct lws *wsi) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    for (auto it = clients.begin(); it != clients.end(); ++it) {
        if (it->second.wsi == wsi) {
            std::cout << it->first << " se ha desconectado.\n";
            clients.erase(it);
            break;
        }
    }
}