#include <libwebsockets.h>
#include <string.h>
#include <iostream>
#include <unordered_map>
#include <vector>
#include "DataSource.cpp"
using namespace std;


const int PORT = 9000;
DataSource sourceoftruth;
unsigned char message_buffer[LWS_PRE + 1024];
void returnUsersToClient(struct lws *wsi) {
    auto users = sourceoftruth.getConnectedUsers();
    
    // Preparar buffer para la respuesta
    unsigned char *buffer = message_buffer + LWS_PRE;
    
    // Tipo de mensaje: USER_LIST_RESPONSE (51)
    buffer[0] = USER_LIST_RESPONSE;
    
    // Número de usuarios
    buffer[1] = static_cast<unsigned char>(users.size());
    
    size_t offset = 2;
    for (const auto& user : users) {
        // Longitud del nombre
        buffer[offset++] = static_cast<unsigned char>(user.username.length());
        
        // Nombre de usuario
        memcpy(buffer + offset, user.username.c_str(), user.username.length());
        offset += user.username.length();
        
        // Estado del usuario
        buffer[offset++] = static_cast<unsigned char>(user.status);
    }
    
    // Enviar respuesta
    lws_write(wsi, buffer, offset, LWS_WRITE_BINARY);
}


static int ws_callback(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len) {
    switch (reason) {
        case LWS_CALLBACK_ESTABLISHED: {
            char rawUsername[50] = {0}; //Creamos esta variable para almacenar el username del usuario conectado
            char *query_string = (char *)lws_get_urlarg_by_name(wsi, "name=", rawUsername, sizeof(rawUsername)); //Obtenemos el username de la sesión del usuario
            std::string username = std::string(rawUsername);
            string realUser = "";
            for (int i = 0; i<username.length(); i++){
                if (i == username.length()-8){
                    break;
                }
                else {
                    realUser += username[i];  
                }
            }
            char ip_address[100] = {0};
            char hostname[100] = {0};
            lws_get_peer_addresses(wsi, -1, hostname, sizeof(hostname), ip_address, sizeof(ip_address));
            bool isConnectionValid = sourceoftruth.insert_user(wsi, realUser, ip_address, 1); //Estado por defecto: Activo
            if (isConnectionValid) {
                std::cout << "User " << realUser << "conectado exitosamente" << std::endl;
            }
            else {
                //Rechazar la solictud si la conexión es
                std::cout << "Rejecting connection from " << rawUsername << " (invalid or duplicate username)" << std::endl;
                lws_close_reason(wsi, LWS_CLOSE_STATUS_GOINGAWAY, (unsigned char*)"Invalid username", strlen("Invalid username"));
                return -1;
            }

            break;
        }
        case LWS_CALLBACK_RECEIVE: {

            if (len < 1) return 0;

            unsigned char *data = (unsigned char*)in; //Obtener el array que envía el cliente
            uint8_t messagetype = data[0];

            switch(messagetype){
                case 1: {
                    returnUsersToClient(wsi);
                }
                case 2: {
                    break;
                }
                case 3: {
                    break;
                }
                case 4: {
                    break;
                }
                case 5:{
                    break;
                }
            }


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