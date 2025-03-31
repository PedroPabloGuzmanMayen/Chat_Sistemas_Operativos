#include <libwebsockets.h>
#include <string.h>
#include <iostream>
#include <unordered_map>
#include <vector>
#include "DataSource.h"
#include "messages.h"
#include "handlers.h"
#include "../server/events.h"
using namespace std;

const int PORT = 9000;
DataSource sourceoftruth;
unsigned char message_buffer[LWS_PRE + 1024];

// Devuelve lista de usuarios conectados
void returnUsersToClient(struct lws *wsi) {
    auto users = sourceoftruth.getConnectedUsers();
    
    unsigned char *buffer = message_buffer + LWS_PRE;
    buffer[0] = USER_LIST_RESPONSE;
    buffer[1] = static_cast<unsigned char>(users.size());

    size_t offset = 2;
    for (const auto& user : users) {
        buffer[offset++] = static_cast<unsigned char>(user.username.length());
        memcpy(buffer + offset, user.username.c_str(), user.username.length());
        offset += user.username.length();
        buffer[offset++] = static_cast<unsigned char>(user.status);
    }
    
    lws_write(wsi, buffer, offset, LWS_WRITE_BINARY);
}

// Actualiza el estado del usuario
void updateUserStatus(struct lws* wsi, uint8_t status) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    for (auto& [username, client] : clients) {
        if (client.wsi == wsi) {
            client.status = status;
            break;
        }
    }
}

// Maneja solicitud de info de otro usuario
void handleGetInfo(struct lws* wsi, const std::vector<uint8_t>& data) {
    std::string target(data.begin(), data.end());

    std::lock_guard<std::mutex> lock(clients_mutex);
    auto it = clients.find(target);
    if (it != clients.end()) {
        const Client& c = it->second;
        std::string info = "Usuario: " + c.username + " | Estado: " + std::to_string(c.status);
        sendBinaryMessage(wsi, INFO_RESPONSE, std::vector<uint8_t>(info.begin(), info.end()));
    } else {
        sendBinaryMessage(wsi, ERROR_MSG, {'U','s','e','r',' ','n','o','t',' ','f','o','u','n','d'});
    }
}

static int ws_callback(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len) {
    switch (reason) {
        case LWS_CALLBACK_ESTABLISHED: {
            char rawUsername[50] = {0};
            lws_get_urlarg_by_name(wsi, "name", rawUsername, sizeof(rawUsername));
            std::cout << "DEBUG: rawUsername = '" << rawUsername 
                      << "', longitud = " << strlen(rawUsername) << std::endl;
            
            std::string realUser;
            const char *prefix = "name=";
            if (strncmp(rawUsername, prefix, strlen(prefix)) == 0) {
                realUser = std::string(rawUsername + strlen(prefix));
            } else {
                realUser = std::string(rawUsername);
            }
            
            char ip_address[100] = {0};
            char hostname[100] = {0};
            lws_get_peer_addresses(wsi, -1, hostname, sizeof(hostname), ip_address, sizeof(ip_address));
            bool isConnectionValid = sourceoftruth.insert_user(wsi, realUser, ip_address, 1); // Estado por defecto: Activo
            if (isConnectionValid) {
                std::cout << "User " << realUser << " conectado exitosamente" << std::endl;
            } else {
                std::cout << "Rejecting connection from " << rawUsername 
                          << " (invalid or duplicate username)" << std::endl;
                lws_close_reason(wsi, LWS_CLOSE_STATUS_GOINGAWAY, 
                                 (unsigned char*)"Invalid username", strlen("Invalid username"));
                return -1;
            }
            break;
        }        
        case LWS_CALLBACK_RECEIVE: {
            if (len < 1) return 0;

            unsigned char *data = (unsigned char*)in;
            uint8_t messagetype = data[0];
            std::vector<uint8_t> payload(data + 1, data + len);

            switch (messagetype) {
                case USER_LIST_REQUEST:
                    returnUsersToClient(wsi);
                    break;

                case GET_INFO:
                    handleGetInfo(wsi, payload);
                    break;

                case GET_HISTORY:
                    handleGetMessageHistory(wsi, payload);
                    break;

                case SEND_MESSAGE:
                    handleSendMessage(wsi, payload);
                    break;

                case SET_STATUS:
                    if (!payload.empty()) {
                        uint8_t status = payload[0];
                        updateUserStatus(wsi, status);
                        std::cout << "DEBUG: Estado actualizado a " << (int)status << std::endl;
                    }
                    break;

                default:
                    std::cout << "DEBUG: Tipo de mensaje desconocido: " << (int)messagetype << std::endl;
                    break;
            }
            break;
        }
    }   
    return 0;
}

static struct lws_protocols protocols[] = {
    {"ws-protocol", ws_callback, 0, 1024}, 
    {NULL, NULL, 0, 0}
};

void startServer(int port) {
    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(info));

    info.port = port;
    info.protocols = protocols;
    info.options = LWS_SERVER_OPTION_HTTP_HEADERS_SECURITY_BEST_PRACTICES_ENFORCE;

    struct lws_context *context = lws_create_context(&info);
    if (!context) {
        std::cerr << "Failed to create WebSocket context" << std::endl;
        return;
    }

    std::cout << "WebSocket server started on port " << port << std::endl;

    while (true) {
        lws_service(context, 0);
    }

    lws_context_destroy(context);
}

int main() {
    startServer(PORT);
    return 0;
}