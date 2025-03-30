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
};
