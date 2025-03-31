// include/DataSource.h
#ifndef DATA_SOURCE_H
#define DATA_SOURCE_H

#include <string>
#include <vector>
#include <unordered_map>
#include <libwebsockets.h>

// Estructura de usuario (definida una única vez)
struct User {
    std::string username;
    std::string ip_addr;
    int status;
};

class DataSource {
public:
    // Método para obtener la lista de usuarios conectados
    std::vector<std::string> getUsernames();

    // Inserta un usuario; retorna true si es exitoso, false si es duplicado o inválido.
    bool insert_user(lws* wsi, const std::string& username, const std::string& ip_addr, int status);

    // Método para obtener los usuarios conectados
    std::vector<User> getConnectedUsers();
};

#endif // DATA_SOURCE_H
