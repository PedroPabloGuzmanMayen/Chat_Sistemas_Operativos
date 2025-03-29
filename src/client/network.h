#ifndef NETWORK_H
#define NETWORK_H

#include <string>
#include <vector>
#include <libwebsockets.h>

// Conecta al servidor usando IP, puerto y nombre de usuario (incrustado en la URL)
bool connectToServer(const std::string &ip, int port, const std::string &username);

// Inicia el loop de servicio (por ejemplo, en un hilo separado)
void startNetworkService();

// Envía un mensaje al servidor (según el protocolo definido)
void sendMessageToServer(uint8_t messageType, const std::vector<uint8_t> &data);

// Procesa el mensaje recibido del servidor
void handleServerMessage(uint8_t messageType, const std::vector<uint8_t> &data);

// Cierra la conexión con el servidor
void disconnectFromServer();

// Variables globales (defínelas en network.cpp)
extern std::vector<std::string> userList;
extern std::vector<std::string> chatHistory;
extern std::string errorMessage;

#endif // NETWORK_H
