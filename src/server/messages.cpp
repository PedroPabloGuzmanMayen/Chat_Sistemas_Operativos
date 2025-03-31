#include "messages.h"
#include "handlers.h"
#include "../server/events.h"
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
void handleSendMessage(struct lws* senderWsi, const std::vector<uint8_t>& data) {
    if (data.size() < 2) return;

    uint8_t targetLen = data[0];
    if (data.size() < 1 + targetLen + 1) return;

    std::string target(data.begin() + 1, data.begin() + 1 + targetLen);
    uint8_t msgLen = data[1 + targetLen];
    std::string message(data.begin() + 1 + targetLen + 1, data.end());

    std::cout << "DEBUG: Received target = '" << target << "' (" << (int)targetLen << " bytes)" << std::endl;

    // Normalizar target por si trae basura o caracteres invisibles
    if (target.find('~') != std::string::npos && target.length() == 1) {
        target = "~";
    }

    std::string senderName = get_username(senderWsi);
    std::string fullMessage = senderName + ": " + message;

    // Guardar el mensaje para el historial
    storeMessage(senderName, fullMessage);

    // Si el mensaje es para el chat general (~), lo enviamos a todos
    if (target == "~") {
        std::cout << "DEBUG: Broadcasting to all users\n";
        for (const auto& [user, client] : clients) {
            std::cout << "Sending to: " << user << "\n";
            sendBinaryMessage(client.wsi, MESSAGE_RECEIVED, std::vector<uint8_t>(fullMessage.begin(), fullMessage.end()));
        }
    } else {
        // Mensaje directo a un usuario específico
        auto it = clients.find(target);
        if (it != clients.end()) {
            std::cout << "DEBUG: Sending direct message to " << target << std::endl;
            sendBinaryMessage(it->second.wsi, MESSAGE_RECEIVED, std::vector<uint8_t>(fullMessage.begin(), fullMessage.end()));
        } else {
            std::cout << "DEBUG: User not found: " << target << std::endl;
            sendBinaryMessage(senderWsi, ERROR_MSG, {'U','s','e','r',' ','n','o','t',' ','f','o','u','n','d'});
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