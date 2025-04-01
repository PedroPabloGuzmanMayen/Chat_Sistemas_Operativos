#include <libwebsockets.h>
#include <string.h>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <functional>
#include "DataSource.cpp"
using namespace std;

// Tamaño del thread pool
const int NUM_THREADS = 8;
const int PORT = 9000;

// Mutex global para proteger el acceso a la fuente de datos
std::mutex dataSourceMutex;

// Buffer global protegido por mutex
std::mutex bufferMutex;
unsigned char message_buffer[LWS_PRE + 1024];

// Fuente de datos compartida
DataSource sourceoftruth;

// Clase para implementar el ThreadPool
class ThreadPool {
public:
    ThreadPool(size_t numThreads);
    ~ThreadPool();

    // Agregar una tarea al pool
    template<class F>
    void enqueue(F&& f);

private:
    // Array de worker threads
    std::vector<std::thread> workers;
    
    // Cola de tareas
    std::queue<std::function<void()>> tasks;
    
    // Sincronización
    std::mutex queueMutex;
    std::condition_variable condition;
    std::atomic<bool> stop;
};

// Constructor del ThreadPool
ThreadPool::ThreadPool(size_t numThreads) : stop(false) {
    for (size_t i = 0; i < numThreads; ++i) {
        workers.emplace_back([this] {
            while (true) {
                std::function<void()> task;
                
                {
                    std::unique_lock<std::mutex> lock(this->queueMutex);
                    
                    // Esperar hasta que haya una tarea o se cierre el pool
                    this->condition.wait(lock, [this] { 
                        return this->stop || !this->tasks.empty(); 
                    });
                    
                    // Salir si se detiene el pool y no hay tareas
                    if (this->stop && this->tasks.empty()) {
                        return;
                    }
                    
                    // Tomar una tarea de la cola
                    task = std::move(this->tasks.front());
                    this->tasks.pop();
                }
                
                // Ejecutar la tarea
                task();
            }
        });
    }
}

// Destructor del ThreadPool
ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        stop = true;
    }
    
    // Despertar a todos los threads para que terminen
    condition.notify_all();
    
    // Esperar a que terminen todos los threads
    for (std::thread& worker : workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

// Agregar una tarea al pool
template<class F>
void ThreadPool::enqueue(F&& f) {
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        
        // No aceptar tareas si el pool está detenido
        if (stop) {
            throw std::runtime_error("enqueue on stopped ThreadPool");
        }
        
        // Agregar la tarea a la cola
        tasks.emplace(std::forward<F>(f));
    }
    
    // Notificar a un thread que hay una nueva tarea
    condition.notify_one();
}

// Crear el thread pool global
ThreadPool* threadPool = nullptr;

// Funciones thread-safe

bool isValidUserStatus(int value) {
    return value == DISCONNECTED || value == ACTIVE || value == BUSY || value == INACTIVE;
}

void returnUsersToClient(struct lws *wsi) {
    std::vector<User> users;
    
    // Sección crítica: obtener usuarios conectados
    {
        std::lock_guard<std::mutex> lock(dataSourceMutex);
        users = sourceoftruth.getConnectedUsers();
    }
    
    // Preparar buffer para la respuesta (protegido por mutex)
    unsigned char* buffer;
    {
        std::lock_guard<std::mutex> lock(bufferMutex);
        buffer = message_buffer + LWS_PRE;
        
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
        
        // Calcular longitud total del mensaje
        size_t totalLength = offset;
        
        // Enviar respuesta (no necesitamos el mutex aquí ya que lws_write es thread-safe para un wsi específico)
        lws_write(wsi, buffer, totalLength, LWS_WRITE_BINARY);
    }
}

void returnSingleUserToClient(struct lws *wsi, const string& username) {
    // Obtener información del usuario (protegido por mutex)
    User* userToreturn = nullptr;
    {
        std::lock_guard<std::mutex> lock(dataSourceMutex);
        userToreturn = sourceoftruth.get_user(username);
    }
    
    // Verificar si el usuario existe
    if (!userToreturn) {
        // Si el usuario no existe, enviar un mensaje de error
        unsigned char* buffer;
        {
            std::lock_guard<std::mutex> lock(bufferMutex);
            buffer = message_buffer + LWS_PRE;
            buffer[0] = ERROR; // Código de error (50)
            buffer[1] = USER_NOT_FOUND; // Código de error específico (1)
            lws_write(wsi, buffer, 2, LWS_WRITE_BINARY);
        }
        return;
    }
    
    // Si el usuario existe, preparar la respuesta (protegido por mutex)
    {
        std::lock_guard<std::mutex> lock(bufferMutex);
        unsigned char* buffer = message_buffer + LWS_PRE;
        
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
        
        // Enviar respuesta
        lws_write(wsi, buffer, total_length, LWS_WRITE_BINARY);
    }
}

void changeUserStatus(struct lws *wsi, int newStatus) {
    User* user_to_change = nullptr;
    std::vector<User> connectedUsers;
    
    // Sección crítica: cambiar el estado del usuario
    {
        std::lock_guard<std::mutex> lock(dataSourceMutex);
        
        user_to_change = sourceoftruth.get_user_by_wsi(wsi);
        if (!user_to_change){
            std::lock_guard<std::mutex> bufferLock(bufferMutex);
            unsigned char *buffer = message_buffer + LWS_PRE;
            buffer[0] = ERROR;
            buffer[1] = USER_NOT_FOUND;
            lws_write(wsi, buffer, 2, LWS_WRITE_BINARY);
            return;
        }
        else if (!isValidUserStatus(newStatus)) {
            //Si el estado no está en los posibles estado, enviar error
            std::lock_guard<std::mutex> bufferLock(bufferMutex);
            unsigned char *buffer = message_buffer + LWS_PRE;
            buffer[0] = ERROR;
            buffer[1] = INVALID_STATUS;
            lws_write(wsi, buffer, 2, LWS_WRITE_BINARY);
            return;
        }
        
        // Actualizar el estado
        sourceoftruth.changeStatus(wsi, newStatus);
        
        // Obtener la lista de usuarios conectados
        connectedUsers = sourceoftruth.getConnectedUsers();
    }
    
    // Preparar el mensaje para enviar a todos los usuarios
    unsigned char* buffer;
    size_t total_length;
    
    {
        std::lock_guard<std::mutex> bufferLock(bufferMutex);
        buffer = message_buffer + LWS_PRE;
        buffer[0] = USER_STATUS_CHANGED;
        buffer[1] = static_cast<unsigned char>(user_to_change->username.length());
        memcpy(buffer + 2, user_to_change->username.c_str(), user_to_change->username.length());
        buffer[2 + user_to_change->username.length()] = static_cast<unsigned char>(user_to_change->status);
        total_length = 2 + user_to_change->username.length() + 1;
    }
    
    // Enviar a todos los usuarios (fuera de la sección crítica para evitar bloqueos prolongados)
    for (const auto& user : connectedUsers) {
        lws* userWsi = nullptr;
        
        {
            std::lock_guard<std::mutex> lock(dataSourceMutex);
            userWsi = sourceoftruth.get_wsi_by_username(user.username);
        }
        
        if (userWsi != nullptr) {
            // Copiar el buffer para cada envío para evitar conflictos
            unsigned char localBuffer[LWS_PRE + 1024];
            memcpy(localBuffer + LWS_PRE, buffer, total_length);
            lws_write(userWsi, localBuffer + LWS_PRE, total_length, LWS_WRITE_BINARY);
        }
    }
}

void sendMessage(struct lws *wsi, string receiver, string content) {
    lws* receiverWsi = nullptr;
    User* sender = nullptr;
    std::vector<User> connectedUsers;
    
    // Sección crítica: obtener datos y guardar el mensaje
    {
        std::lock_guard<std::mutex> lock(dataSourceMutex);
        if (receiver != "~") {
            receiverWsi = sourceoftruth.get_wsi_by_username(receiver);
        }
        sender = sourceoftruth.get_user_by_wsi(wsi);
        
        if ((!receiverWsi && receiver != "~") || !sender) {
            std::lock_guard<std::mutex> bufferLock(bufferMutex);
            unsigned char *buffer = message_buffer + LWS_PRE;
            buffer[0] = ERROR;
            buffer[1] = USER_NOT_FOUND;
            lws_write(wsi, buffer, 2, LWS_WRITE_BINARY);
            return;
        }
        
        // Insertar el mensaje en la fuente de datos
        sourceoftruth.insertMessage(wsi, receiver, content);
        
        if (receiver == "~") {
            connectedUsers = sourceoftruth.getConnectedUsers();
        }
    }
    
    // Preparar el mensaje para enviar
    unsigned char localBuffer[LWS_PRE + 1024];
    unsigned char* buffer = localBuffer + LWS_PRE;
    size_t total_length;
    
    {
        buffer[0] = MESSAGE_RECEIVED;
        buffer[1] = static_cast<unsigned char>(sender->username.length());
        memcpy(buffer + 2, sender->username.c_str(), sender->username.length());
        buffer[2 + sender->username.length()] = static_cast<unsigned char>(content.length());
        memcpy(buffer + 3 + sender->username.length(), content.c_str(), content.length());
        total_length = 3 + sender->username.length() + content.length();
    }
    
    // Enviar el mensaje
    if (receiver == "~") {
        // Enviar a todos los usuarios conectados
        for (const auto& user : connectedUsers) {
            lws* userWsi = nullptr;
            
            {
                std::lock_guard<std::mutex> lock(dataSourceMutex);
                userWsi = sourceoftruth.get_wsi_by_username(user.username);
            }
            
            if (userWsi != nullptr) {
                // Crear una copia del buffer para cada envío
                unsigned char sendBuffer[LWS_PRE + 1024];
                memcpy(sendBuffer + LWS_PRE, buffer, total_length);
                lws_write(userWsi, sendBuffer + LWS_PRE, total_length, LWS_WRITE_BINARY);
            }
        }
    } else {
        // Enviar solo al destinatario
        lws_write(receiverWsi, buffer, total_length, LWS_WRITE_BINARY);
    }
}

void sendMessageHistory(struct lws *wsi, string nameChat) {
    std::vector<ChatMessage> messages_to_send;
    User* currentUser = nullptr;
    std::string currentUsername;
    
    // Sección crítica: obtener el historial de mensajes
    {
        std::lock_guard<std::mutex> lock(dataSourceMutex);
        currentUser = sourceoftruth.get_user_by_wsi(wsi);
        currentUsername = currentUser ? currentUser->username : "";
        messages_to_send = sourceoftruth.getChatHistory(nameChat, currentUsername);
    }
    
    if (messages_to_send.size() > 0) {
        // Construir el buffer de respuesta
        unsigned char localBuffer[LWS_PRE + 4096]; // Buffer más grande para historiales largos
        unsigned char* buffer = localBuffer + LWS_PRE;
        int pos = 0;
        
        buffer[pos++] = CHAT_HISTORY_RESPONSE;
        buffer[pos++] = messages_to_send.size();
        
        for (const auto& msg : messages_to_send) {
            buffer[pos++] = msg.sender.length();
            memcpy(buffer + pos, msg.sender.c_str(), msg.sender.length());
            pos += msg.sender.length();
            
            buffer[pos++] = msg.content.length();
            memcpy(buffer + pos, msg.content.c_str(), msg.content.length());
            pos += msg.content.length();
        }
        
        // Enviar la respuesta
        lws_write(wsi, buffer, pos, LWS_WRITE_BINARY);
    } else {
        // Enviar mensaje de error
        unsigned char localBuffer[LWS_PRE + 2];
        unsigned char* buffer = localBuffer + LWS_PRE;
        buffer[0] = ERROR;
        buffer[1] = USER_NOT_FOUND;
        lws_write(wsi, buffer, 2, LWS_WRITE_BINARY);
    }
}

// Estructura para pasar datos al callback
struct PerSessionData {
    bool initialized;
};

static int ws_callback(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len) {
    PerSessionData *pss = static_cast<PerSessionData*>(user);
    
    switch (reason) {
        case LWS_CALLBACK_ESTABLISHED: {
            if (!pss) return -1;
            pss->initialized = true;
            
            char rawUsername[50] = {0};
            lws_get_urlarg_by_name(wsi, "name=", rawUsername, sizeof(rawUsername));
            std::string username = std::string(rawUsername);
            string realUser = "";
            
            for (int i = 0; i < username.length(); i++) {
                if (i == username.length() - 8) {
                    break;
                } else {
                    realUser += username[i];
                }
            }
            
            char ip_address[100] = {0};
            char hostname[100] = {0};
            lws_get_peer_addresses(wsi, -1, hostname, sizeof(hostname), ip_address, sizeof(ip_address));
            
            bool isConnectionValid;
            {
                std::lock_guard<std::mutex> lock(dataSourceMutex);
                isConnectionValid = sourceoftruth.insert_user(wsi, realUser, ip_address, 1);
            }
            
            if (isConnectionValid) {
                // Programar la tarea de cambiar el estado en el thread pool
                threadPool->enqueue([wsi]() {
                    changeUserStatus(wsi, 1); // Activo
                    std::cout << "Usuario conectado exitosamente" << std::endl;
                });
            } else {
                std::cout << "Rejecting connection from " << rawUsername << " (invalid or duplicate username)" << std::endl;
                lws_close_reason(wsi, LWS_CLOSE_STATUS_GOINGAWAY, (unsigned char*)"Invalid username", strlen("Invalid username"));
                return -1;
            }
            
            break;
        }
        
        case LWS_CALLBACK_RECEIVE: {
            if (len < 1) return 0;
            if (!pss || !pss->initialized) return 0;
            
            // Copiar los datos recibidos en un buffer local
            unsigned char *data = new unsigned char[len];
            memcpy(data, in, len);
            size_t dataLen = len;
            
            // Programar el procesamiento de la solicitud en el thread pool
            threadPool->enqueue([wsi, data, dataLen]() {
                uint8_t messagetype = data[0];
                
                switch(messagetype) {
                    case 1: // Listar usuarios
                        returnUsersToClient(wsi);
                        break;
                        
                    case 2: { // Obtener información de un usuario
                        string userTofind = "";
                        for (int i = 2; i < data[1] + 2; i++) {
                            userTofind += char(data[i]);
                        }
                        returnSingleUserToClient(wsi, userTofind);
                        break;
                    }
                    
                    case 3: // Cambiar estado
                        changeUserStatus(wsi, data[dataLen-1]);
                        break;
                        
                    case 4: { // Enviar mensaje
                        if (dataLen < 2) break;
                        
                        uint8_t dest_len = data[1];
                        if (dataLen < 2 + dest_len) break;
                        
                        string userToSend = "";
                        for (int i = 2; i < 2 + dest_len; i++) {
                            userToSend += char(data[i]);
                        }
                        
                        uint8_t msg_len = data[2 + dest_len];
                        if (dataLen < 3 + dest_len + msg_len) break;
                        
                        string message_content = "";
                        for (int i = 3 + dest_len; i < 3 + dest_len + msg_len; i++) {
                            message_content += char(data[i]);
                        }
                        
                        sendMessage(wsi, userToSend, message_content);
                        break;
                    }
                    
                    case 5: { // Solicitar historial de chat
                        string chatName = "";
                        for (int i = 2; i < data[1] + 2; i++) {
                            chatName += char(data[i]);
                        }
                        sendMessageHistory(wsi, chatName);
                        break;
                    }
                }
                
                delete[] data;
            });
            
            break;
        }
        
        case LWS_CALLBACK_CLOSED: {
            if (pss) {
                pss->initialized = false;
            }
            
            threadPool->enqueue([wsi]() {
                std::lock_guard<std::mutex> lock(dataSourceMutex);
                User* user = sourceoftruth.get_user_by_wsi(wsi);
                if (user) {
                    std::cout << "Usuario " << user->username << " desconectado" << std::endl;
                    sourceoftruth.changeStatus(wsi, DISCONNECTED);
                }
            });
            
            break;
        }
    }
    
    return 0;
}

static struct lws_protocols protocols[] = {
    {"ws-protocol", ws_callback, sizeof(PerSessionData), 1024}, 
    {NULL, NULL, 0, 0}
};

void startServer(int port) {
    // Inicializar el thread pool
    threadPool = new ThreadPool(NUM_THREADS);
    
    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(info));
    
    info.port = port;
    info.protocols = protocols;
    info.options = LWS_SERVER_OPTION_HTTP_HEADERS_SECURITY_BEST_PRACTICES_ENFORCE;

    info.count_threads = 1; 
    
    struct lws_context *context = lws_create_context(&info);
    if (!context) {
        std::cerr << "Failed to create WebSocket context" << std::endl;
        delete threadPool;
        return;
    }
    
    std::cout << "WebSocket server started on port " << port << " with " << NUM_THREADS << " worker threads" << std::endl;
    
    while (true) {
        lws_service(context, 50); 
    }
    lws_context_destroy(context);
    delete threadPool;
}

int main() {
    startServer(PORT);
    return 0;
}