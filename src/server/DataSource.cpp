// src/server/DataSource.cpp
#include "DataSource.h"
#include "handlers.h"  // Para acceder al mapa global de clientes
#include <iostream>
#include <algorithm>
#include <mutex>

// Devuelve una lista de todos los nombres de usuarios conectados
std::vector<std::string> DataSource::getUsernames() {
    std::vector<std::string> usernames;
    std::lock_guard<std::mutex> lock(clients_mutex);
    for (const auto& [name, _] : clients) {
        usernames.push_back(name);
    }
    return usernames;
}

// Inserta un nuevo usuario al mapa global 'clients'
bool DataSource::insert_user(lws* wsi, const std::string& username, const std::string& ip_addr, int status) {
    if (username == "~" || username.empty() || username.length() > 10)
        return false;

    std::lock_guard<std::mutex> lock(clients_mutex);
    if (clients.find(username) != clients.end()) return false;

    clients[username] = Client{username, wsi, status};
    return true;
}

// Retorna la lista de usuarios conectados
std::vector<User> DataSource::getConnectedUsers() {
    std::vector<User> list;
    std::lock_guard<std::mutex> lock(clients_mutex);
    for (const auto& [name, client] : clients) {
        list.push_back(User{name, "0.0.0.0", client.status, client.wsi});
    }
    return list;
}