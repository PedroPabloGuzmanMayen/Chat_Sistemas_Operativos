#include "network.h"
#include "events.h"  // Define MESSAGE_RECEIVED, etc.
#include <libwebsockets.h>
#include <thread>
#include <mutex>
#include <iostream>
#include <cstring>
#include <vector>
#include <FL/Fl.H>

// Variables globales internas para la conexión
static struct lws_context* g_context = nullptr;
static struct lws* g_client_wsi = nullptr;
static std::thread g_serviceThread;
static bool g_running = false;
static std::mutex g_mutex;

// Variables globales declaradas en network.h
std::vector<std::string> userList;
std::vector<std::string> chatHistory;
std::string errorMessage;

// Callback del cliente para manejar los eventos de la conexión
static int callback_client(struct lws *wsi, enum lws_callback_reasons reason,
                           void *user, void *in, size_t len) {
    switch (reason) {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            std::cout << "Debug: Connection established with server." << std::endl;
            break;
        case LWS_CALLBACK_CLIENT_RECEIVE: {
            if (len < 2) break;
            uint8_t* received = static_cast<uint8_t*>(in);
            uint8_t msgType = received[0];
            uint8_t dataSize = received[1];
            std::vector<uint8_t> data(received + 2, received + 2 + dataSize);
            std::cout << "Debug: Received message of type " << (int)msgType << " from server." << std::endl;
            handleServerMessage(msgType, data);
            break;
        }
        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            std::cerr << "Debug: Connection error." << std::endl;
            break;
        case LWS_CALLBACK_CLIENT_CLOSED:
            std::cout << "Debug: Connection closed." << std::endl;
            g_client_wsi = nullptr;
            g_running = false;
            break;
        default:
            break;
    }
    return 0;
}

// Arreglo de protocolos para el cliente (debe coincidir el nombre con el del servidor)
static struct lws_protocols protocols[] = {
    {
        "ws-protocol",
        callback_client,
        0,
        1024,
        0,          // id
        nullptr,    // user
        0           // tx_packet_size
    },
    { NULL, NULL, 0, 0, 0, nullptr, 0 }
};

bool connectToServer(const std::string &ip, int port, const std::string &username) {
    std::cout << "Debug: Attempting connection to " << ip << ":" << port 
              << " with username: " << username << std::endl;
    
    // Crear contexto sin escuchar (cliente)
    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(info));
    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = protocols;
    
    g_context = lws_create_context(&info);
    if (!g_context) {
        std::cerr << "Error: Failed to create WebSocket context." << std::endl;
        return false;
    }
    
    // Configurar la conexión del cliente
    struct lws_client_connect_info ccinfo;
    memset(&ccinfo, 0, sizeof(ccinfo));
    ccinfo.context  = g_context;
    ccinfo.address  = ip.c_str();
    ccinfo.port     = port;
    
    // Construir la URL con el parámetro "name"
    std::string path = "/?name=" + username;
    ccinfo.path     = path.c_str();
    
    ccinfo.host     = lws_canonical_hostname(g_context);
    ccinfo.origin   = "origin";
    ccinfo.protocol = protocols[0].name;
    ccinfo.ssl_connection = 0; // Sin SSL

    g_client_wsi = lws_client_connect_via_info(&ccinfo);
    if (!g_client_wsi) {
        std::cerr << "Error: Could not connect to server." << std::endl;
        return false;
    }
    return true;
}

void startNetworkService() {
    g_running = true;
    g_serviceThread = std::thread([](){
        while (g_running) {
            lws_service(g_context, 50);
        }
    });
}

void sendMessageToServer(uint8_t messageType, const std::vector<uint8_t> &data) {
    if (!g_client_wsi) {
        std::cerr << "Error: No active connection to server." << std::endl;
        return;
    }
    // Construir el mensaje: [messageType][dataSize][data...]
    std::vector<uint8_t> buffer;
    buffer.push_back(messageType);
    buffer.push_back(static_cast<uint8_t>(data.size()));
    buffer.insert(buffer.end(), data.begin(), data.end());
    
    size_t totalSize = LWS_PRE + buffer.size();
    unsigned char* buf = new unsigned char[totalSize];
    memset(buf, 0, LWS_PRE);
    memcpy(buf + LWS_PRE, buffer.data(), buffer.size());
    
    int n = lws_write(g_client_wsi, buf + LWS_PRE, buffer.size(), LWS_WRITE_BINARY);
    if(n < (int)buffer.size()) {
        std::cerr << "Error: Could not send full message." << std::endl;
    }
    delete[] buf;
}

// Declaración de un callback que actualiza la UI (defínelo en client.cpp o en otro módulo compartido)
extern void refreshChatDisplay();

void handleServerMessage(uint8_t messageType, const std::vector<uint8_t> &data) {
    if (messageType == MESSAGE_RECEIVED) {
        std::string msg(data.begin(), data.end());
        chatHistory.push_back(msg);
        std::cout << "Debug: New message received: " << msg << std::endl;
        // Notificar a la UI para que se actualice:
        Fl::awake([](void*){
            refreshChatDisplay();
        });
    } else {
        std::cout << "Debug: Received unknown message type: " << (int)messageType << std::endl;
    }
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