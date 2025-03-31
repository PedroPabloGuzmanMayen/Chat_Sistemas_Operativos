#pragma once
#include <string>
#include <vector>
#include <libwebsockets.h>

// Estructura para representar un usuario (ligera, no depende de Client)
struct User {
    std::string username;
    std::string ip_address;
    int status;
    struct lws* wsi;  // opcional
};

class DataSource {
public:
    std::vector<std::string> getUsernames();
    bool insert_user(lws* wsi, const std::string& username, const std::string& ip_addr, int status);
    std::vector<User> getConnectedUsers();
};