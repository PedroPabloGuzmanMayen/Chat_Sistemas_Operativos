#ifndef NETWORK_H
#define NETWORK_H

#include <vector>
#include <string>
#include <libwebsockets.h>

// Funciones de red
bool connectToServer(const std::string &ip, int port);
void handleServerMessage(uint8_t messageType, const std::vector<uint8_t> &data);
void sendMessageToServer(uint8_t messageType, const std::vector<uint8_t> &data);

extern std::vector<std::string> userList;
extern std::vector<std::string> chatHistory;
extern std::string errorMessage;

#endif