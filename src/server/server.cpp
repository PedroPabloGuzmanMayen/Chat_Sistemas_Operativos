#include <libwebsockets.h>
#include <string.h>
#include <iostream>
#include <unordered_map>
#include <vector>
#include "DataSource.cpp"
#include "logger.cpp"
#include <chrono>  
#include <thread>

using namespace std;

void changeUserStatus(struct lws *wsi, int newStatus);

const int PORT = 9000;
DataSource sourceoftruth;
unsigned char message_buffer[LWS_PRE + 1024];
bool isValidUserStatus(int value) {
    return value == DISCONNECTED || value == ACTIVE || value == BUSY || value == INACTIVE;
}
//Crear el logger global
Logger serverLogger("logs.txt");

void check_inactive_users(int timeout_seconds) {
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(10)); // Verificar cada 10 segundos
        cout << "Verficiando inactividad...\n" << endl;
        auto inactive_users = sourceoftruth.get_inactive_users(timeout_seconds);
        for (auto& [wsi, username] : inactive_users) {
            // En lugar de pasar a DESCONECTADO, pasar a INACTIVO
            cout << "usuario: " << username << " inactivo" << endl;
            sourceoftruth.changeStatus(wsi, INACTIVE);
            // Enviar notificación de cambio de estado a todos los clientes
            changeUserStatus(wsi, INACTIVE);
            serverLogger.log(username + " cambió su estado a INACTIVO por inactividad");
        }
    }
}

void returnUsersToClient(struct lws *wsi) {
    serverLogger.log("Un usuario consultó la lista de usuarios");
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

void returnSingleUserToClient(struct lws *wsi, const string& username) {
    // Obtener información del usuario

    User* userToreturn = sourceoftruth.get_user(username);
    
    // Verificar si el usuario existe
    if (!userToreturn) {
        // Si el usuario no existe, enviar un mensaje de error
        unsigned char *buffer = message_buffer + LWS_PRE;
        buffer[0] = ERROR; // Código de error (50)
        buffer[1] = USER_NOT_FOUND; // Código de error específico (1)
        lws_write(wsi, buffer, 2, LWS_WRITE_BINARY);
        serverLogger.log("ERROR, el usuario  " + username + "  no fue encontrado");
        return;
    }
    
    // Si el usuario existe, preparar la respuesta
    unsigned char *buffer = message_buffer + LWS_PRE;
    
    // Tipo de mensaje: USER_INFO_RESPONSE (52)
    buffer[0] = USER_INFO_RESPONSE;
    
    // Longitud del nombre de usuario
    buffer[1] = static_cast<unsigned char>(userToreturn->username.length());
    
    // Copiar el nombre de usuario
    memcpy(buffer + 2, userToreturn->username.c_str(), userToreturn->username.length());
    
    // Agregar el estado del usuario después del nombre
    buffer[2 + userToreturn->username.length()] = static_cast<unsigned char>(userToreturn->status);
    
    // Calcular el tamaño total del mensaje: 1 byte tipo + 1 byte longitud + N bytes nombre + 1 byte estado
    size_t total_length = 2 + userToreturn->username.length() + 1;
    serverLogger.log("Se consultó la información del usuario: " + username);
    // Enviar respuesta
    lws_write(wsi, buffer, total_length, LWS_WRITE_BINARY);
}

void changeUserStatus(struct lws *wsi, int newStatus) { //Función para cambiar el estado de un usuario. 
    
    User* user_to_change = sourceoftruth.get_user_by_wsi(wsi);
    if (!user_to_change){
        unsigned char *buffer = message_buffer + LWS_PRE;
        buffer[0] = ERROR; // Código de error (50)
        buffer[1] = USER_NOT_FOUND; // Código de error específico (1)
        serverLogger.log("ERROR, el usuario  " + user_to_change->username + "  no fue encontrado");
        lws_write(wsi, buffer, 2, LWS_WRITE_BINARY);
        return;
    }
    else if (!isValidUserStatus(newStatus)) {
        //Si el estado no está en los posibles estado, enviar error
        unsigned char *buffer = message_buffer + LWS_PRE;
        buffer[0] = ERROR; // Código de error (50)
        buffer[1] = INVALID_STATUS; // Código de error específico (1)
        serverLogger.log("ERROR, el status era inválido");
        lws_write(wsi, buffer, 2, LWS_WRITE_BINARY);
        return;
    }
    unsigned char *buffer = message_buffer + LWS_PRE;
    sourceoftruth.changeStatus(wsi, newStatus);

    buffer[0] = USER_STATUS_CHANGED;
    buffer[1] = static_cast<unsigned char>(user_to_change->username.length());
    memcpy(buffer + 2, user_to_change->username.c_str(), user_to_change->username.length()); //Copiar el username
    string statusString = "";
    if (newStatus == 0 || newStatus == 3){
        statusString = "DESCONECTADO";
    }
    else if (newStatus == 1){
        statusString = "ACTIVO";
    }
    else if (newStatus == 2){
        statusString = "OCUPADO";
    }
    serverLogger.log(user_to_change->username + "Cambio su estado a: " + statusString );
    buffer[2 + user_to_change->username.length()] = static_cast<unsigned char>(user_to_change->status); //Agregar el estado
    auto connectedUsers = sourceoftruth.getConnectedUsers();
    size_t total_length = 2 + user_to_change->username.length() + 1;
    //Enviar a todos los usuarios
    for (const auto& user : connectedUsers) {
        // Obtener el WSI asociado a cada usuario
            lws* userWsi = sourceoftruth.get_wsi_by_username(user.username);
            if (userWsi != nullptr) { //Verificar si existe el usuario
                lws_write(userWsi, buffer, total_length, LWS_WRITE_BINARY);
            }
    }
}

void sendMessage(struct lws *wsi, string reciever, string content){
    User* sender = sourceoftruth.get_user_by_wsi(wsi);
    unsigned char *buffer = message_buffer + LWS_PRE;
    if (!sender) {
        if (reciever == "~"){
            if (sender->status == DISCONNECTED){
                buffer[0] = ERROR; // Código de error (50)
                buffer[1] = USER_DISCONNECTED; // Código de error específico (1)
                serverLogger.log("ERROR, el usuario " + reciever + "estaba desconectado y no pudo enviar el mensaje");
                lws_write(wsi, buffer, 2, LWS_WRITE_BINARY);
                return;
            }
            sourceoftruth.insertMessage(wsi, reciever, content);
            buffer[0] = MESSAGE_RECEIVED;
            buffer[1] = static_cast<unsigned char>(sender->username.length());
            memcpy(buffer + 2, sender->username.c_str(), sender->username.length());
            buffer[2+sender->username.length()] = static_cast<unsigned char>(content.length()); //Len del mensaje
            memcpy(buffer + 3 +sender->username.length(), content.c_str(), content.length() +1); // Contenido del mensaje
            size_t total_length = 2 + sender->username.length() + content.length() +1;
            serverLogger.log(sender->username + "  envió un mensaje al chat general");
            auto connectedUsers = sourceoftruth.getConnectedUsers();
            for (const auto& user : connectedUsers) {
            // Obtener el WSI asociado a cada usuario
                lws* userWsi = sourceoftruth.get_wsi_by_username(user.username);
                if (userWsi != nullptr) { //Verificar si existe el usuario
                    lws_write(userWsi, buffer, total_length, LWS_WRITE_BINARY);
                }
            }
        }
        else {
            User * receiver = sourceoftruth.get_user(reciever);
            if (!receiver){
                unsigned char *buffer = message_buffer + LWS_PRE;
                buffer[0] = ERROR; // Código de error (50)
                buffer[1] = USER_NOT_FOUND; // Código de error específico (1)
                serverLogger.log("ERROR, el usuario  " + reciever + "  no fue encontrado");
                lws_write(wsi, buffer, 2, LWS_WRITE_BINARY);
                return; 
            }
            else if (receiver->status == BUSY){
                unsigned char *buffer = message_buffer + LWS_PRE;
                buffer[0] = ERROR; // Código de error (50)
                buffer[1] = USER_DISCONNECTED; // Código de error específico (1)
                serverLogger.log("ERROR, el usuario " + reciever + "estaba ocupado y no pudo recibir el mensaje");
                lws_write(wsi, buffer, 2, LWS_WRITE_BINARY);
                return;
            }
            else if (sender->status == DISCONNECTED){
                unsigned char *buffer = message_buffer + LWS_PRE;
                buffer[0] = ERROR; // Código de error (50)
                buffer[1] = USER_DISCONNECTED; // Código de error específico (1)
                serverLogger.log("ERROR, el usuario " + reciever + "estaba desconectado y no pudo enviar el mensaje");
                lws_write(wsi, buffer, 2, LWS_WRITE_BINARY);
                return;
            }
            struct lws* receiverWsi = sourceoftruth.get_wsi_by_username(reciever);
            sourceoftruth.insertMessage(wsi, reciever, content);
            buffer[0] = MESSAGE_RECEIVED;
            buffer[1] = static_cast<unsigned char>(sender->username.length());
            memcpy(buffer + 2, sender->username.c_str(), sender->username.length());
            buffer[2+sender->username.length()] = static_cast<unsigned char>(content.length()); //Len del mensaje
            memcpy(buffer + 3 +sender->username.length(), content.c_str(), content.length() +1); // Contenido del mensaje
            size_t total_length = 2 + sender->username.length() + content.length() +1;
            serverLogger.log(sender->username + "  envió un mensaje privado a:  " + reciever);
            lws_write(receiverWsi, buffer, total_length, LWS_WRITE_BINARY); //Enviar al usuario interesado
        }
    }
}
    

void notifyNewConnection(string username){
    unsigned char *buffer = message_buffer + LWS_PRE;
    buffer[0] = USER_STATUS_CHANGED;
    buffer[1] = static_cast<unsigned char>(username.length());
    memcpy(buffer + 2, username.c_str(), username.length());
    buffer[2 + username.length()] = USER_STATUS_CHANGED;
    size_t total_length = 2 + username.length() + 1; 
    auto connectedUsers = sourceoftruth.getConnectedUsers();
    for (const auto& user : connectedUsers) {
        // Obtener el WSI asociado a cada usuario
            lws* userWsi = sourceoftruth.get_wsi_by_username(user.username);
            if (userWsi != nullptr) { //Verificar si existe el usuario
                lws_write(userWsi, buffer, total_length, LWS_WRITE_BINARY);
            }
    }

    
}

void sendChatHistory(lws *wsi,string chatName){

        // Buffer para la respuesta, considerando espacio LWS_PRE
    unsigned char buffer[LWS_PRE + 1024];
    unsigned char *p = &buffer[LWS_PRE];
    
    // El primer byte indica el tipo de mensaje: CHAT_HISTORY_RESPONSE (56)
    *p++ = CHAT_HISTORY_RESPONSE;
    
    // Obtener el usuario solicitante
    User* requestingUser = sourceoftruth.get_user_by_wsi(wsi);
    if (!requestingUser) {
        // Si no encontramos al usuario, enviar error
        buffer[LWS_PRE] = ERROR;
        buffer[LWS_PRE + 1] = USER_NOT_FOUND;
        lws_write(wsi, &buffer[LWS_PRE], 2, LWS_WRITE_BINARY);
        return;
    }
    
    // Identificar de qué chat obtener el historial
    vector<ChatMessage> messages;
    if (chatName == "~") {
        // Chat general
        messages = sourceoftruth.getChatHistory("~");
    } else {
        // Chat privado, generar la clave correcta
        string reciever = chatName;
        string senderName = requestingUser->username;
        string chatKey = (senderName < reciever) ? senderName + ":" + reciever : reciever + ":" + senderName;
        messages = sourceoftruth.getChatHistory(chatKey);
    }
    
    // Reiniciar el puntero después del tipo de mensaje
    p = &buffer[LWS_PRE + 1];
    
    size_t numMessages = std::min(messages.size(), (size_t)255);
    *p++ = (unsigned char)numMessages;
    
    // Variables para seguimiento
    size_t remainingSpace = 1022; // 1024 - 2 bytes ya usados (tipo y num mensajes)
    size_t messagesSent = 0;
    
    // Iterar sobre los mensajes
    for (size_t i = 0; i < numMessages; i++) {
        const ChatMessage& msg = messages[i];
        
        // Longitud del nombre del usuario
        size_t userLen = msg.sender.length();
        // Longitud del contenido del mensaje
        size_t msgLen = msg.content.length();
        
        // Verificar si hay espacio suficiente
        if (remainingSpace < 2 + userLen + msgLen) {
            break; // No hay más espacio, detener
        }
        
        // Añadir longitud del nombre
        *p++ = (unsigned char)userLen;
        remainingSpace--;
        
        // Añadir nombre del usuario
        memcpy(p, msg.sender.c_str(), userLen);
        p += userLen;
        remainingSpace -= userLen;
        
        // Añadir longitud del mensaje
        *p++ = (unsigned char)msgLen;
        remainingSpace--;
        
        // Añadir contenido del mensaje
        memcpy(p, msg.content.c_str(), msgLen);
        p += msgLen;
        remainingSpace -= msgLen;
        
        messagesSent++;
    }
    
    // Corregir número de mensajes si se envían menos por limitaciones de espacio
    if (messagesSent < numMessages) {
        buffer[LWS_PRE + 1] = (unsigned char)messagesSent;
    }
    
    // Calcular el tamaño total del mensaje
    size_t totalLen = p - &buffer[LWS_PRE];
    
    // Enviar el paquete al cliente
    lws_write(wsi, &buffer[LWS_PRE], totalLen, LWS_WRITE_BINARY);

}


static int ws_callback(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len) {
    switch (reason) {
        case LWS_CALLBACK_ESTABLISHED: {
            char rawUsername[50] = {0};
            // Se pide el argumento "name" (sin el signo '=')
            lws_get_urlarg_by_name(wsi, "name", rawUsername, sizeof(rawUsername));
            std::string realUser(rawUsername);
            const std::string prefix = "name=";
            if (realUser.compare(0, prefix.size(), prefix) == 0) {
                realUser = realUser.substr(prefix.size());
            }
            std::cout << "DEBUG: realUser = '" << realUser 
                      << "', longitud = " << realUser.length() << std::endl;
            
            char ip_address[100] = {0};
            char hostname[100] = {0};
            lws_get_peer_addresses(wsi, lws_get_socket_fd(wsi), hostname, sizeof(hostname), ip_address, sizeof(ip_address));
            bool isConnectionValid = sourceoftruth.insert_user(wsi, realUser, ip_address, 1); // Estado por defecto: Activo
            if (isConnectionValid) {
                std::cout << "User " << realUser << " conectado exitosamente" << std::endl;
                serverLogger.log("Un " + realUser + " salvaje se conecto al server!");
                notifyNewConnection(realUser);
            }
            else {
                std::cout << "Rejecting connection from " << realUser << " (invalid or duplicate username)" << std::endl;
                serverLogger.log("Se rechazo la conexión de: " + realUser + " por usuario inválido o duplicado!");
                lws_close_reason(wsi, LWS_CLOSE_STATUS_GOINGAWAY,
                                 (unsigned char*)"Invalid username", strlen("Invalid username"));
                return -1;
            }
            break;
        }
        case LWS_CALLBACK_RECEIVE: {

            if (len < 1) return 0;
            sourceoftruth.update_user_activity(wsi);

            unsigned char *data = (unsigned char*)in; //Obtener el array que envía el cliente
            uint8_t messagetype = data[0];

            switch(messagetype){
                case 1: { //Caso 1: listar usuarios
                    returnUsersToClient(wsi);
                    break;
                }
                case 2: { //Caso 2: 
                    string userTofind = ""; //Castear el contenido del array
                    for (int i = 2; i < data[1] +2; i++){
                        userTofind += char(data[i]);
                    }
                    cout << "username: " << userTofind << "\n" << endl;
                    returnSingleUserToClient(wsi, userTofind);
                    break;
                }
                case 3: {
                    changeUserStatus(wsi, data[len-1]);
                    break;
                }
                case 4: {
                    if (len < 2) return 0;
    
                    // Obtener longitud del nombre del destinatario
                    uint8_t dest_len = data[1];
    
                    // Verificar que haya suficientes datos para el nombre
                    if (len < 2 + dest_len) return 0;
    
                    // Extraer nombre del destinatario
                    string userToSend = "";
                    for (int i = 2; i < 2 + dest_len; i++) {
                        userToSend += char(data[i]);
                    }
    
                    // Obtener longitud del mensaje (ubicada después del nombre)
                    uint8_t msg_len = data[2 + dest_len];
    
                    // Verificar que haya suficientes datos para el mensaje
                    if (len < 3 + dest_len + msg_len) return 0;
    
                    // Extraer contenido del mensaje
                    string message_content = "";
                    for (int i = 3 + dest_len; i < 3 + dest_len + msg_len; i++) {
                        message_content += char(data[i]);
                    }
                    sendMessage(wsi,userToSend, message_content);
                    break;
                }
                case 5:{
                    string chatName = "";
                    for (int i = 2; i < data[1] + 2; i++) {
                        chatName += char(data[i]);
                    }
                    sendChatHistory(wsi, chatName);
                    break;
                }
            }

        break; 
        }
        case LWS_CALLBACK_CLOSED: {
            std::cout << "DEBUG: Conexión cerrada, removiendo usuario" << std::endl;
            sourceoftruth.removeUser(wsi);
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

    info.timeout_secs = 0;

    info.port = port;
    info.protocols = protocols;
    info.options = LWS_SERVER_OPTION_HTTP_HEADERS_SECURITY_BEST_PRACTICES_ENFORCE;
    info.count_threads = 4;
    

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
    const int INACTIVITY_TIMEOUT = 60; // 5 minutos en segundos
    
    std::thread inactivity_thread(check_inactive_users, INACTIVITY_TIMEOUT);
    inactivity_thread.detach();
    startServer(PORT);
    return 0;
}