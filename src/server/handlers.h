#pragma once
#include <libwebsockets.h>
#include <vector>
#include <unordered_map>
#include <string>
#include <mutex>

struct Client {
    std::string username;          // ← Nombre del usuario (clave para identificarlo)
    struct lws* wsi;               // Socket asociado
    int status;                    // Estado (Activo, Ocupado, etc.)
};

// Variables compartidas (usadas en múltiples archivos)
extern std::unordered_map<std::string, Client> clients;
extern std::mutex clients_mutex;

// Funciones
void sendBinaryMessage(struct lws* wsi, uint8_t messageType, const std::vector<uint8_t>& data);
void handleReceivedMessage(struct lws* wsi, uint8_t messageType, const std::vector<uint8_t>& data);
void handleClientDisconnection(struct lws* wsi);
std::string get_username(struct lws* wsi);