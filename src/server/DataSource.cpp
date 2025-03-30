#include <libwebsockets.h>
#include <string.h>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <set>
#include <map>
#include <unordered_set>

using namespace std;

struct User {
    string username;
    string ip_addr;
    int status;
};

enum UserStatus {
    DISCONNECTED = 0,
    ACTIVE = 1,
    BUSY = 2,
    INACTIVE = 3
};

// Códigos de mensajes del cliente al servidor
enum ClientMessageType {
    LIST_USERS = 1,
    GET_USER = 2,
    CHANGE_STATUS = 3,
    SEND_MESSAGE = 4,
    GET_CHAT_HISTORY = 5
};

// Códigos de mensajes del servidor al cliente
enum ServerMessageType {
    ERROR = 50,
    USER_LIST_RESPONSE = 51,
    USER_INFO_RESPONSE = 52,
    USER_REGISTERED = 53,
    USER_STATUS_CHANGED = 54,
    MESSAGE_RECEIVED = 55,
    CHAT_HISTORY_RESPONSE = 56
};

// Códigos de error
enum ErrorType {
    USER_NOT_FOUND = 1,
    INVALID_STATUS = 2,
    EMPTY_MESSAGE = 3,
    USER_DISCONNECTED = 4
};

struct ChatMessage {
    std::string sender;
    std::string receiver;  // Puede ser "~" para chat general
    std::string content;
};

class DataSource {
    private:
        unordered_map<lws*, User> users; // Nos ayuda a asociar una sesión con un usuario
        vector<ChatMessage> generalChat; //Vector que contiene los mensajes del chat general
        map<unordered_set<string>, vector<ChatMessage>> privateChats;

    public:
        vector<string> getUsernames() {
            vector<string> usernames;
            for (const auto& pair : users) {
                usernames.push_back(pair.second.username);
            }
            return usernames;
        }

        bool insert_user(lws* wsi, const string& username, const string& ip_addr, int status) {
            // Verificar si el username existe o es válido
            if (username == "~" || username == "" || username.length() > 10) {
                return false;
            }
            for (const auto& pair : users) {
                if (pair.second.username == username) {
                    return false; 
                }
            }
            // Si no existe o no es un nombre inválido, insertar 
            users[wsi] = {username, ip_addr, status};
            return true;
        }
        //Método para hallar los usuarios que están conectados
        std::vector<User> getConnectedUsers() {
            std::vector<User> connected;
            for (const auto& [username, user] : users) {
                if (user.status != DISCONNECTED) {
                    connected.push_back(user);
                }
            }
            return connected;
        }
};
