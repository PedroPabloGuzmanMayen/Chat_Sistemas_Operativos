// src/server/handlers.cpp
#include "handlers.h"
#include <iostream>
#include <cstring>

// Definición de las variables externas
std::unordered_map<std::string, Client> clients;
std::mutex clients_mutex;

// Implementación de sendBinaryMessage para el servidor
void sendBinaryMessage(struct lws *wsi, uint8_t messageType, const std::vector<uint8_t> &data) {
    std::vector<uint8_t> buffer;
    buffer.push_back(messageType);
    buffer.push_back(static_cast<uint8_t>(data.size()));
    buffer.insert(buffer.end(), data.begin(), data.end());

    size_t totalSize = LWS_PRE + buffer.size();
    unsigned char* buf = new unsigned char[totalSize];
    memset(buf, 0, LWS_PRE);
    memcpy(buf + LWS_PRE, buffer.data(), buffer.size());

    int n = lws_write(wsi, buf + LWS_PRE, buffer.size(), LWS_WRITE_BINARY);
    if(n < (int)buffer.size()) {
        std::cerr << "Server: Error: Could not send full message." << std::endl;
    }
    delete[] buf;
}

// Recuperar el nombre de usuario asociado a un wsi
std::string get_username(struct lws* wsi) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    for (const auto& [name, client] : clients) {
        if (client.wsi == wsi)
            return name;
    }
    return "desconocido";
}

void handleReceivedMessage(struct lws *wsi, uint8_t messageType, const std::vector<uint8_t> &data) {
    // Implementación según el protocolo
}

void handleClientDisconnection(struct lws *wsi) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    for (auto it = clients.begin(); it != clients.end(); ++it) {
        if (it->second.wsi == wsi) {
            std::cout << "Usuario desconectado: " << it->first << std::endl;
            clients.erase(it);
            break;
        }
    }
}