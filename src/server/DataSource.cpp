#include <libwebsockets.h>
#include <string.h>
#include <iostream>
#include <unordered_map>
#include <vector>

using namespace std;

struct User {
    string username;
    string ip_addr;
    int status;
};

class DataSource {
    private:
        std::unordered_map<lws*, User> users; // Nos ayuda a asociar una sesión con un usuario

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
            if (username == "~" || username.size() == 0) {
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
