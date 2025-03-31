// messages.h
#pragma once
#include <libwebsockets.h>
#include <vector>
#include <string>

void handleSendMessage(struct lws* wsi, const std::vector<uint8_t>& data);
void handleGetMessageHistory(struct lws* wsi, const std::vector<uint8_t>& data);
void handleListUsers(struct lws* wsi);
void storeMessage(const std::string& username, const std::string& message);