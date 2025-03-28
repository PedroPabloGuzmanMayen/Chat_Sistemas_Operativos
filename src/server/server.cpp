#include <libwebsockets.h>
#include <string.h>
#include <iostream>
#include <unordered_map>
#include <vector>
#include "DataSource.cpp"
using namespace std;


const int PORT = 9000;
DataSource sourceoftruth;


static int ws_callback(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len) {
    switch (reason) {
        case LWS_CALLBACK_ESTABLISHED: {
            char username[50] = {0}; //Creamos esta variable para almacenar el username del usuario conectado
            char *query_string = (char *)lws_get_urlarg_by_name(wsi, "name=", username, sizeof(username)); //Obtenemos el username de la sesión del usuario
            char ip_address[100] = {0};
            char hostname[100] = {0};
            lws_get_peer_addresses(wsi, -1, hostname, sizeof(hostname), ip_address, sizeof(ip_address));
            bool isConnectionValid = sourceoftruth.insert_user(wsi, username, ip_address, 1); //Estado por defecto: Activo

            if (isConnectionValid) {
                
            }
            else {

            }

            break;
        }
        case LWS_CALLBACK_RECEIVE: {

            if (len > 0){
                
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