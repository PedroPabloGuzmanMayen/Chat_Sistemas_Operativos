#include "users.h"
#include "handlers.h"
#include <iostream>
#include <mutex>
#include <regex>

extern std::unordered_map<std::string, Client> clients;
extern std::mutex clients_mutex;
std::regex validUsernameRegex("^[A-Za-z0-9_-]{3,16}$");

/**
 * Maneja el registro de usuarios con validaciones
 */
void handleRegisterUser(struct lws *wsi, const std::vector<uint8_t> &data) {
    std::string username(data.begin(), data.end());

    if (username == "~" || username.empty() || !std::regex_match(username, validUsernameRegex)) {
        std::cout << "Error: Nombre de usuario inválido.\n";
        sendBinaryMessage(wsi, 50, {1}); // Código de error 1: Nombre inválido
        return;
    }

    std::lock_guard<std::mutex> lock(clients_mutex);
    if (clients.find(username) == clients.end()) {
        clients[username] = {username, wsi, ACTIVE};
        std::cout << username << " se ha conectado.\n";
        sendBinaryMessage(wsi, 53, {username.begin(), username.end(), 1}); // Mensaje 53: Nuevo usuario
    } else {
        std::cout << "Usuario duplicado: " << username << "\n";
        sendBinaryMessage(wsi, 50, {2}); // Código de error 2: Usuario duplicado
    }
}

/**
 * Maneja el cambio de estado de un usuario
 */
void handleChangeStatus(struct lws *wsi, const std::vector<uint8_t> &data) {
    if (data.size() < 2) {
        sendBinaryMessage(wsi, 50, {3}); // Error 3: Datos insuficientes
        return;
    }

    std::string username(data.begin(), data.end() - 1);
    uint8_t newStatus = data.back();

    std::lock_guard<std::mutex> lock(clients_mutex);
    if (clients.find(username) != clients.end()) {
        clients[username].status = static_cast<UserStatus>(newStatus);
        sendBinaryMessage(wsi, 54, {username.begin(), username.end(), newStatus});
    } else {
        sendBinaryMessage(wsi, 50, {4}); // Código de error 4: Usuario no encontrado
    }
}