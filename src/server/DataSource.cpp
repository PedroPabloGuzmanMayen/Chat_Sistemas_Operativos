#include <libwebsockets.h>
#include <string.h>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <set>
#include <map>
#include <unordered_set>
#include <mutex>
#include <shared_mutex>

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
    std::string receiver;
    std::string content;
};

class DataSource {
    private:
        // Mutex para acceso concurrente a los datos
        mutable shared_mutex mutex_; // Permite múltiples lectores o un único escritor
        
        unordered_map<lws*, User> users; // Asocia una sesión con un usuario
        vector<ChatMessage> generalChat; // Vector que contiene los mensajes del chat general
        map<string, vector<ChatMessage>> privateChats;

    public:
        vector<string> getUsernames() {
            // Lock de lectura ya que solo leemos datos
            shared_lock<shared_mutex> lock(mutex_);
            
            vector<string> usernames;
            for (const auto& pair : users) {
                usernames.push_back(pair.second.username);
            }
            return usernames;
        }

        bool insert_user(lws* wsi, const string& username, const string& ip_addr, int status) {
            // Lock exclusivo ya que modificamos datos
            unique_lock<shared_mutex> lock(mutex_);
            
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
        
        // Método para hallar los usuarios que están conectados
        std::vector<User> getConnectedUsers() {
            // Lock de lectura
            shared_lock<shared_mutex> lock(mutex_);
            
            std::vector<User> connected;
            for (const auto& pair : users) {
                if (pair.second.status != DISCONNECTED) {
                    connected.push_back(pair.second);
                }
            }
            return connected;
        }
        
        // Función para hallar un usuario
        User* get_user(const std::string& username) {
            // Lock de lectura
            shared_lock<shared_mutex> lock(mutex_);
            
            // Iterar hasta encontrar el usuario que buscamos
            for (auto& pair : users) {
                if (pair.second.username == username) {
                    // Devolver una copia para evitar problemas de concurrencia
                    return &(pair.second);
                }
            }
            return nullptr;
        }

        void changeStatus(struct lws *wsi, int newStatus) {
            // Lock exclusivo
            unique_lock<shared_mutex> lock(mutex_);
            
            auto it = users.find(wsi);
            if (it != users.end()) {
                it->second.status = newStatus;
            }
        }

        void insertMessage(struct lws *wsi, string receiver, string messageContent) {
            // Lock exclusivo
            unique_lock<shared_mutex> lock(mutex_);
            
            auto it = users.find(wsi);
            if (it == users.end()) {
                return; // No se encontró el usuario
            }
            
            string senderName = it->second.username;
            
            if (receiver == "~") {
                generalChat.push_back({senderName, receiver, messageContent});
            } else {
                string chatKey = (senderName < receiver) ? senderName + ":" + receiver : receiver + ":" + senderName;
                privateChats[chatKey].push_back({senderName, receiver, messageContent});
            }
        }
        
        lws* get_wsi_by_username(const std::string& username) {
            // Lock de lectura
            shared_lock<shared_mutex> lock(mutex_);
            
            for (const auto& pair : users) {
                if (pair.second.username == username) {
                    return pair.first; 
                }
            }
            return nullptr;
        }
        
        User* get_user_by_wsi(struct lws *wsi) {
            // Lock de lectura
            shared_lock<shared_mutex> lock(mutex_);
            
            auto it = users.find(wsi);
            if (it != users.end()) {
                return &(it->second);
            }
            return nullptr;
        }

        vector<ChatMessage> getChatHistory(string name, string currentUser = "") {
            // Lock de lectura
            shared_lock<shared_mutex> lock(mutex_);
            
            if (name == "~") {
                return generalChat;
            } else {
                string chatKey;
                if (currentUser.empty()) {
                    if (privateChats.find(name) != privateChats.end()) {
                        return privateChats[name];
                    }
                    for (const auto& pair : privateChats) {
                        if (pair.first.find(name) != string::npos) {
                            return pair.second;
                        }
                    }
                    return vector<ChatMessage>();
                } else {
                    chatKey = (currentUser < name) ? currentUser + ":" + name : name + ":" + currentUser;
                }
            }
        }
};
