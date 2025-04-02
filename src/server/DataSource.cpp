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
#include <chrono>
#include <thread>
using namespace std;

struct User {
    string username;
    std::string ip_addr; 
    int status;
    std::chrono::time_point<std::chrono::system_clock> last_activity;
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
        map<string, vector<ChatMessage>> privateChats;
        mutable std::mutex data_mutex;
        std::shared_mutex mutex_;

    public:
        vector<string> getUsernames() {
            lock_guard<mutex> lock(data_mutex);
            vector<string> usernames;
            for (const auto& pair : users) {
                usernames.push_back(pair.second.username);
            }
            return usernames;
        }

        bool insert_user(lws* wsi, const std::string& username, const std::string& ip_addr, int status) {
            std::lock_guard<std::shared_mutex> lock(mutex_);
            std::cout << "DEBUG: insert_user: username='" << username << "', ip_addr='" << ip_addr << "'" << std::endl;
        
            // Verificar si el username es inválido
            if (username == "~" || username.empty() || username.length() > 10) {
                std::cout << "DEBUG: Username inválido: '" << username << "'" << std::endl;
                return false;
            }
        
            // Buscar si ya existe un usuario con ese nombre
            for (auto it = users.begin(); it != users.end(); ++it) {
                if (it->second.username == username) {
                    // Si el usuario existe y está desconectado, permitir reconexión.
                    if (it->second.status == DISCONNECTED) {
                        std::cout << "DEBUG: Usuario '" << username << "' estaba desconectado. Se permite reconexión." << std::endl;
                        // Eliminar la entrada antigua
                        users.erase(it);
                        break;
                    } else {
                        std::cout << "DEBUG: Usuario '" << username << "' ya existe y está activo." << std::endl;
                        return false;
                    }
                }
            }
        
            // Insertar el nuevo usuario
            users[wsi] = {username, ip_addr, status, std::chrono::system_clock::now()};
            return true;
        }
        
        
        //Método para hallar los usuarios que están conectados
        std::vector<User> getConnectedUsers() {
            lock_guard<mutex> lock(data_mutex);
            std::vector<User> connected;
            for (const auto& [username, user] : users) {
                if (user.status != DISCONNECTED) {
                    connected.push_back(user);
                }
            }
            return connected;
        }
        //Función para hallar un usuario
        User* get_user(const std::string& username) {
            lock_guard<mutex> lock(data_mutex);
            //Iterar hasta encontrar el usuario que buscamos
            for (auto& pair : users) {
                if (pair.second.username == username) {
                    return &(pair.second);
                }
            }
            return nullptr;
        }

        void changeStatus(struct lws *wsi, int newStatus) { //Función para cambiar el status de un usuario
            lock_guard<mutex> lock(data_mutex);
            users[wsi].status = newStatus;
        }

        void removeUser(lws* wsi) {
            std::lock_guard<std::shared_mutex> lock(mutex_);
            auto it = users.find(wsi);
            if (it != users.end()) {
                std::cout << "DEBUG: Removiendo usuario '" << it->second.username << "'" << std::endl;
                users.erase(it);
            }
        }

        void insertMessage(struct lws *wsi, string reciever, string messageContent){
            lock_guard<mutex> lock(data_mutex);
            string senderName = users[wsi].username;
            if (reciever == "~"){
                generalChat.push_back({senderName, reciever, messageContent}); //Insertar el nuevo mensaje
            }
            else {
                string chatKey = (senderName < reciever) ? senderName + ":" + reciever : reciever + ":" + senderName; //Generar llava única para el chat privado
                privateChats[chatKey].push_back({senderName, reciever, messageContent}); //Insertar mensaje en la conversación privada
            }
        }
        lws* get_wsi_by_username(const std::string& username) { //Busca el wsi de un user
            lock_guard<mutex> lock(data_mutex);
            for (const auto& pair : users) {
                if (pair.second.username == username) {
                    return pair.first; 
                }
            }
            return nullptr;
        }
        User* get_user_by_wsi(struct lws *wsi) {
            lock_guard<mutex> lock(data_mutex);
            //Iterar hasta encontrar el usuario que buscamos
            auto it = users.find(wsi);
            if (it != users.end()) {
                return &(it->second);
            }
            return nullptr;
        } 

        vector<ChatMessage> getChatHistory(string chatKey){
            lock_guard<mutex> lock(data_mutex);
            if(chatKey == "~"){
                return generalChat;
            }
            else {
                if (privateChats.find(chatKey) != privateChats.end()) {
                    return privateChats[chatKey];
                } else {
                    // Si no existe, devolver un vector vacío
                    return vector<ChatMessage>();
                }
            }
        }
        void update_user_activity(lws* wsi) {
            lock_guard<mutex> lock(data_mutex);
            auto it = users.find(wsi);
            if (it != users.end()) {
                it->second.last_activity = std::chrono::system_clock::now();
            }
        }
        vector<pair<lws*, string>> get_inactive_users(int timeout_seconds) {
            lock_guard<mutex> lock(data_mutex);
            vector<pair<lws*, string>> inactive_users;
            auto now = std::chrono::system_clock::now();
            
            for (auto& pair : users) {
                auto duration = std::chrono::duration_cast<std::chrono::seconds>(
                    now - pair.second.last_activity);
                if (duration.count() > timeout_seconds && pair.second.status != DISCONNECTED) {
                    inactive_users.emplace_back(pair.first, pair.second.username);
                }
            }
            
            return inactive_users;
        }
};
