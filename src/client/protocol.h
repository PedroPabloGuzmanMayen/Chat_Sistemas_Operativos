#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <string>
#include <vector>
#include <cstdint>

// IDs de mensajes Cliente → Servidor
enum ClientMessageID : uint8_t {
    LIST_USERS = 1,
    GET_USER_INFO = 2,
    CHANGE_STATUS = 3,
    SEND_MESSAGE = 4,
    GET_HISTORY = 5
};

// IDs de mensajes Servidor → Cliente
enum ServerMessageID : uint8_t {
    ERROR_MSG = 50,
    USERS_LIST = 51,
    USER_INFO = 52,
    USER_REGISTERED = 53,
    STATUS_CHANGED = 54,
    NEW_MESSAGE = 55,
    CHAT_HISTORY = 56
};

// Estados posibles del usuario
enum UserStatus : uint8_t {
    DISCONNECTED = 0,
    ACTIVE = 1,
    BUSY = 2,
    INACTIVE = 3
};

// Construcción de mensajes
namespace Protocol {

    std::vector<uint8_t> make_list_users();

    std::vector<uint8_t> make_get_user_info(const std::string& username);

    std::vector<uint8_t> make_change_status(const std::string& username, UserStatus status);

    std::vector<uint8_t> make_send_message(const std::string& to, const std::string& message);

    std::vector<uint8_t> make_get_history(const std::string& chat_name);

    // Utilidades internas
    void append_string(std::vector<uint8_t>& buffer, const std::string& str);
    std::string extract_string(const std::vector<uint8_t>& data, size_t& offset);

}

#endif // PROTOCOL_H