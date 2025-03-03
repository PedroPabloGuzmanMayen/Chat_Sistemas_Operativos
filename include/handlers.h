#ifndef HANDLERS_H
#define HANDLERS_H

#include <unordered_map>
#include <vector>
#include <string>
#include <mutex>
#include <libwebsockets.h>
#include "users.h"
#include "messages.h"

// Funciones de manejo de eventos
void sendBinaryMessage(struct lws *wsi, uint8_t messageType, const std::vector<uint8_t> &data);
void handleReceivedMessage(struct lws *wsi, uint8_t messageType, const std::vector<uint8_t> &data);
void handleClientDisconnection(struct lws *wsi);

#endif