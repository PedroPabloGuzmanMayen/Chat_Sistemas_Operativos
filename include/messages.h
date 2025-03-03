#ifndef MESSAGES_H
#define MESSAGES_H

#include <vector>
#include <string>
#include <libwebsockets.h>

// Manejo de mensajes
void handleListUsers(struct lws *wsi);
void handleSendMessage(struct lws *wsi, const std::vector<uint8_t> &data);
void handleGetMessageHistory(struct lws *wsi, const std::vector<uint8_t> &data);
void storeMessage(const std::string &username, const std::string &message);

#endif