#ifndef USERS_H
#define USERS_H

#include <unordered_map>
#include <string>
#include <mutex>
#include <libwebsockets.h>

// Estados de usuario
enum UserStatus { DISCONNECTED = 0, ACTIVE = 1, BUSY = 2, INACTIVE = 3 };

// Estructura de Cliente
struct Client {
    std::string username;
    struct lws *wsi;
    UserStatus status;
};

// Funciones de usuarios
void handleRegisterUser(struct lws *wsi, const std::vector<uint8_t> &data);
void handleChangeStatus(struct lws *wsi, const std::vector<uint8_t> &data);

#endif