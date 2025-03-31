#include "messages.h"
#include "handlers.h"
#include "events.h"
#include <iostream>
#include <mutex>
#include <map>

// Historial de mensajes por usuario
std::map<std::string, std::vector<std::string>> messageHistory;
extern std::unordered_map<std::string, Client> clients;
extern std::mutex clients_mutex;

/**
 * Guarda un mensaje en el historial del chat
 */
void storeMessage(const std::string &username, const std::string &message) {
    messageHistory[username].push_back(message);
}

/**
 * Envía la lista de usuarios conectados al cliente que lo solicita
 */
void handleListUsers(struct lws *wsi) {
    std::vector<uint8_t> userList;
    userList.push_back(clients.size());

    std::lock_guard<std::mutex> lock(clients_mutex);
    for (const auto &[name, client] : clients) {
        userList.push_back(name.size());
        userList.insert(userList.end(), name.begin(), name.end());
        userList.push_back(client.status);
    }

    sendBinaryMessage(wsi, 51, userList);
}

/**
 * Maneja el envío de mensajes entre usuarios
 */
void handleSendMessage(struct lws *wsi, const std::vector<uint8_t> &data) {
    size_t separator = data[0];
    std::string targetUser(data.begin(), data.begin() + separator);
    std::string message(data.begin() + separator, data.end());
    
    std::lock_guard<std::mutex> lock(clients_mutex);
    if (targetUser == "~") { // Chat general: enviar a todos los clientes conectados
        for (const auto &pair : clients) {
            sendBinaryMessage(pair.second.wsi, MESSAGE_RECEIVED, std::vector<uint8_t>(message.begin(), message.end()));
        }
        // También puedes almacenar el mensaje en un historial global si es necesario.
        storeMessage("~", message);
    } else {
        // Mensaje directo
        if (clients.find(targetUser) != clients.end()) {
            storeMessage(targetUser, message); // Guardar en historial
            sendBinaryMessage(clients[targetUser].wsi, MESSAGE_RECEIVED, std::vector<uint8_t>(message.begin(), message.end()));
        } else {
            // Enviar error: usuario no encontrado (código de error 4)
            sendBinaryMessage(wsi, ERROR_MSG, {4});
        }
    }
}


/**
 * Envía el historial de mensajes a un usuario que lo solicita
 */
void handleGetMessageHistory(struct lws *wsi, const std::vector<uint8_t> &data) {
    std::string username(data.begin(), data.end());

    if (messageHistory.find(username) != messageHistory.end()) {
        std::vector<uint8_t> history;
        history.push_back(messageHistory[username].size());

        for (const std::string &msg : messageHistory[username]) {
            history.push_back(msg.size());
            history.insert(history.end(), msg.begin(), msg.end());
        }

        sendBinaryMessage(wsi, 56, history);
    } else {
        sendBinaryMessage(wsi, 50, {4}); // Código de error 4: Usuario sin historial
    }
}