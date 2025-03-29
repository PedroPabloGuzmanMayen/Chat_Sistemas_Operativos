#include "network.h"
#include "events.h"  
#include <libwebsockets.h>
#include <thread>
#include <mutex>
#include <iostream>
#include <cstring>
#include <vector>
#include <FL/Fl.H>

// Variables globales para el cliente
static struct lws_context* g_context = nullptr;
static struct lws* g_client_wsi = nullptr;
static std::thread g_serviceThread;
static bool g_running = false;
static std::mutex g_mutex;

// Definición de las variables globales declaradas en network.h
std::vector<std::string> userList;
std::vector<std::string> chatHistory;
std::string errorMessage;

// Definimos el callback del cliente (única definición)
static int callback_client(struct lws *wsi, enum lws_callback_reasons reason,
                           void *user, void *in, size_t len) {
    switch (reason) {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            std::cout << "Cliente: Conexión establecida con el servidor." << std::endl;
            break;
        case LWS_CALLBACK_CLIENT_RECEIVE: {
            if (len < 2) break;
            uint8_t* received = static_cast<uint8_t*>(in);
            uint8_t msgType = received[0];
            uint8_t dataSize = received[1];
            std::vector<uint8_t> vecData(received + 2, received + 2 + dataSize);
            handleServerMessage(msgType, vecData);
            break;
        }
        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            std::cerr << "Cliente: Error en la conexión." << std::endl;
            break;
        case LWS_CALLBACK_CLIENT_CLOSED:
            std::cout << "Cliente: Conexión cerrada." << std::endl;
            g_client_wsi = nullptr;
            g_running = false;
            break;
        default:
            break;
    }
    return 0;
}

// Definimos los protocolos para el cliente
static struct lws_protocols protocols[] = {
    {
        "ws-protocol",      // Nombre del protocolo (debe coincidir con el del servidor)
        callback_client,
        0,
        1024,
        0,          // .id (inicializado a 0)
        nullptr,    // .user
        0           // .tx_packet_size
    },
    { NULL, NULL, 0, 0, 0, nullptr, 0 }
};

bool connectToServer(const std::string &ip, int port, const std::string &username) {
    // Creamos el contexto de libwebsockets para el cliente
    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(info));
    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = protocols;

    g_context = lws_create_context(&info);
    if (!g_context) {
        std::cerr << "Error: No se pudo crear el contexto de libwebsockets" << std::endl;
        return false;
    }

    // Configuramos la conexión del cliente
    struct lws_client_connect_info ccinfo;
    memset(&ccinfo, 0, sizeof(ccinfo));
    ccinfo.context  = g_context;
    ccinfo.address  = ip.c_str();
    ccinfo.port     = port;
    
    // Construir la URL con el nombre de usuario en la query (nota: usamos "name=" para que coincida con el servidor)
    std::string path = "/?name=" + username;
    ccinfo.path     = path.c_str();
    
    ccinfo.host     = lws_canonical_hostname(g_context);
    ccinfo.origin   = "origin";
    ccinfo.protocol = protocols[0].name;
    ccinfo.ssl_connection = 0; // Sin SSL

    g_client_wsi = lws_client_connect_via_info(&ccinfo);
    if (!g_client_wsi) {
        std::cerr << "Error: No se pudo conectar al servidor" << std::endl;
        return false;
    }
    return true;
}

void startNetworkService() {
    // Inicia el loop de libwebsockets en un hilo separado
    g_running = true;
    g_serviceThread = std::thread([](){
        while (g_running) {
            lws_service(g_context, 50);
        }
    });
}

void sendMessageToServer(uint8_t messageType, const std::vector<uint8_t> &data) {
    if (!g_client_wsi) {
        std::cerr << "Error: No hay conexión activa con el servidor." << std::endl;
        return;
    }
    // Construir el mensaje: [messageType][dataSize][data...]
    std::vector<uint8_t> buffer;
    buffer.push_back(messageType);
    buffer.push_back(static_cast<uint8_t>(data.size()));
    buffer.insert(buffer.end(), data.begin(), data.end());
    // Libwebsockets requiere un buffer con LWS_PRE bytes de padding
    size_t totalSize = LWS_PRE + buffer.size();
    unsigned char* buf = new unsigned char[totalSize];
    memset(buf, 0, LWS_PRE);
    memcpy(buf + LWS_PRE, buffer.data(), buffer.size());
    int n = lws_write(g_client_wsi, buf + LWS_PRE, buffer.size(), LWS_WRITE_BINARY);
    if(n < static_cast<int>(buffer.size())) {
        std::cerr << "Error: No se pudo enviar el mensaje completo." << std::endl;
    }
    delete[] buf;
}

void handleServerMessage(uint8_t messageType, const std::vector<uint8_t> &data) {
    // Ejemplo: si se recibe un mensaje (ID 55)
    if (messageType == MESSAGE_RECEIVED) {
        std::string msg(data.begin(), data.end());
        chatHistory.push_back(msg);
        // Notifica a la UI (FLTK) que hay un nuevo mensaje usando Fl::awake
        Fl::awake([](void*) {
            // Aquí se puede actualizar el buffer del chat en la UI.
            // Por ejemplo, llamar a una función de refresco.
        });
    }
    // Otros casos se pueden manejar aquí...
}

void disconnectFromServer() {
    g_running = false;
    if (g_serviceThread.joinable())
        g_serviceThread.join();
    if (g_context) {
        lws_context_destroy(g_context);
        g_context = nullptr;
    }
}