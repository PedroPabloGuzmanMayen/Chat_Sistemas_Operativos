#ifndef NETWORK_H
#define NETWORK_H

#include <string>
#include <vector>
#include <libwebsockets.h>

// Funciones de red para el cliente
bool connectToServer(const std::string &ip, int port, const std::string &username);
void startNetworkService();
void sendMessageToServer(uint8_t messageType, const std::vector<uint8_t> &data);
void handleServerMessage(uint8_t messageType, const std::vector<uint8_t> &data);
void disconnectFromServer();

// Variables globales (para almacenar datos recibidos, etc.)
extern std::vector<std::string> userList;
extern std::vector<std::string> chatHistory;
extern std::string errorMessage;

#endif // NETWORK_H
