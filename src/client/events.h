#pragma once
#include <vector>
#include <string>

constexpr uint8_t SEND_MESSAGE       = 10;
constexpr uint8_t MESSAGE_RECEIVED   = 55;
constexpr uint8_t ERROR_MSG          = 50;
constexpr uint8_t USER_LIST_REQUEST  = 11;
constexpr uint8_t USER_LIST_RESPONSE = 51;
constexpr uint8_t GET_HISTORY        = 12;
constexpr uint8_t HISTORY_RESPONSE   = 52;
constexpr uint8_t GET_INFO           = 13;
constexpr uint8_t INFO_RESPONSE      = 53;
constexpr uint8_t SET_STATUS         = 14;
constexpr uint8_t STATUS_OK          = 54;

// ✅ Solo aquí: definición inline
inline std::vector<std::string> chatHistory;