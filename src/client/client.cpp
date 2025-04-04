// src/client/client.cpp

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Text_Buffer.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Browser.H>
#include <FL/Fl_Box.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Pack.H>

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <queue>
#include "events.h"


namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;


// Estados de usuario
// Desconectado = 0, Activo = 1, Ocupado = 2, Inactivo = 3
namespace chat {
    enum UserStatus {
        DISCONNECTED = 0,
        ACTIVE = 1,
        BUSY = 2,
        INACTIVE = 3
    };
}

// Forward declaration
class ChatWindow;

// Clase WebSocketClient adaptada para la integración con FLTK
class WebSocketClient {
private:
    net::io_context ioc_;
    websocket::stream<beast::tcp_stream> ws_;
    std::string username_;
    std::mutex mutex_;
    bool connected_ = false;
    std::thread ioc_thread_;
    ChatWindow* chat_window_ = nullptr;
    net::executor_work_guard<net::io_context::executor_type> work_guard_;

public:
    WebSocketClient(const std::string& username)
        : ioc_()
        , ws_(ioc_)
        , username_(username), 
        work_guard_(net::make_work_guard(ioc_)) {  
    }

    ~WebSocketClient() {
        close();
        if (ioc_thread_.joinable()) {
            ioc_thread_.join();
        }
    }

    void setChatWindow(ChatWindow* cw) {
        chat_window_ = cw;
    }

    // Conectar al servidor WebSocket
    bool connect(const std::string& host, const std::string& port);

    // Solicitar lista de usuarios
    void requestUserList();

    // Obtener información de un usuario específico
    void requestUserInfo(const std::string& targetUser);

    // Enviar un mensaje a otro usuario
    void sendMessageToUser(const std::string& targetUser, const std::string& message);

    // Cambiar estado de usuario
    void setStatus(chat::UserStatus status);

    // Hilo para ejecutar el contexto de IO
    void runIoContext();

    // Hilo para recibir mensajes del servidor
    void runReceiveLoop();

    // Manejar los diferentes tipos de mensajes recibidos
    void handleMessage(uint8_t messageType, const uint8_t* data, size_t length);

    // Cerrar la conexión
    void close();

    bool isConnected() const {
        return connected_;
    }
};

// Clase para la ventana de login
class LoginWindow : public Fl_Window {
    Fl_Input *user_in, *ip_in, *port_in;
    Fl_Button *connect_btn;

    static void on_connect(Fl_Widget* w, void* data);

public:
    LoginWindow(): Fl_Window(300, 180, "Conectar al Chat") {
        user_in = new Fl_Input(100, 20, 180, 25, "Usuario:");
        ip_in = new Fl_Input(100, 60, 180, 25, "IP Servidor:");
        ip_in->value("127.0.0.1");
        port_in = new Fl_Input(100, 100, 180, 25, "Puerto:");
        port_in->value("9000");
        connect_btn = new Fl_Button(100, 140, 100, 30, "Conectar");
        connect_btn->callback(on_connect, this);
        end();
    }
};

// Clase para la ventana de chat
class ChatWindow : public Fl_Window {
    friend class WebSocketClient;
    
    Fl_Text_Display *chat_disp;
    Fl_Text_Buffer *chat_buf;
    Fl_Input *msg_in;
    Fl_Button *send_btn;
    Fl_Choice *status_choice;
    Fl_Browser *user_list;
    Fl_Box *info_label;
    Fl_Button *list_btn, *info_btn;
    Fl_Input *info_input;
    Fl_Choice *target_choice;
    Fl_Pack *right;
    
    WebSocketClient* client_;
    std::string username_;
    std::mutex append_mutex_, uiQueueMutex;
    std::queue<std::string> uiMessageQueue;


    static void on_send(Fl_Widget* w, void* data);
    static void on_list_users(Fl_Widget* w, void* data);
    static void on_get_info(Fl_Widget* w, void* data);
    static void on_status_change(Fl_Widget* w, void* data);

public:
    ChatWindow(const std::string &user, const std::string &ip, int port);
    
    ~ChatWindow() {
        Fl::remove_idle(ChatWindow::idle_cb, this);
        delete client_;
    }

    void enqueueMessage(const std::string &msg) {
        std::lock_guard<std::mutex> lock(uiQueueMutex);
        uiMessageQueue.push(msg);
        Fl::awake();
    }

    void processPendingMessages() {
        std::lock_guard<std::mutex> lock(uiQueueMutex);
        while (!uiMessageQueue.empty()) {
            std::string msg = uiMessageQueue.front();
            uiMessageQueue.pop();
            chat_buf->append((msg + "\n").c_str());
        }
        chat_disp->insert_position(chat_buf->length());
        chat_disp->show_insert_position();
    }
    

    static void updateUserListCallback(void* data) {
        ChatWindow* cw = static_cast<ChatWindow*>(data);
        if (cw->client_->isConnected()) {
            cw->client_->requestUserList();
        }
        // Reprogramar la actualización en 5 segundos
        Fl::repeat_timeout(5.0, updateUserListCallback, cw);
    }

    //  idle_cb para llama a processPendingMessages:
    static void idle_cb(void* data) {
        ChatWindow* cw = static_cast<ChatWindow*>(data);
        cw->processPendingMessages();
    }

    void appendToChat(const std::string& text) {
        // Esta función puede ser llamada desde otro hilo, así que necesitamos
        // usar Fl::lock() y Fl::unlock() para hacer thread-safe las operaciones de FLTK
        std::string fullText = text + "\n";
        std::cout << "[DEBUG] appendToChat: " << fullText << std::endl;
        
        Fl::lock();
        chat_buf->append(fullText.c_str());
        // Desplazarse al final
        chat_disp->insert_position(chat_buf->length());
        chat_disp->show_insert_position();
        Fl::unlock();
        Fl::awake();
    }

    void updateUserList(const std::vector<std::pair<std::string, uint8_t>>& users);
    void addUserToTargetList(const std::string& username);
    
    // Solo para mensaje privado
    std::string getSelectedTargetUser() {
        int idx = target_choice->value();
        if (idx == 0) { // Suponiendo que el primer elemento es "Chat general"
            std::cout << "[DEBUG] Se seleccionó Chat General (retornando '~')" << std::endl;
            return "~";
        }
        std::string destinatario = target_choice->text(idx);
        std::cout << "[DEBUG] Se seleccionó destinatario: " << destinatario << std::endl;
        return destinatario;
    }
};

// Implementaciones

// WebSocketClient
bool WebSocketClient::connect(const std::string& host, const std::string& port) {
    try {
        // Buscar el host
        tcp::resolver resolver(ioc_);
        auto results = resolver.resolve(host, port);

        // Conectar al servidor
        auto ep = beast::get_lowest_layer(ws_).connect(results);

        // Actualizar el host string para WebSocket handshake
        std::string host_str = host + ':' + std::to_string(ep.port());

        // Establecer el handshake HTTP
        ws_.set_option(websocket::stream_base::timeout::suggested(beast::role_type::client));
        ws_.handshake(host_str, "/?" + std::string("name=") + username_);

        connected_ = true;
        
        // Iniciar el hilo para el contexto de IO y el bucle de recepción
        ioc_thread_ = std::thread([this]() { runIoContext(); });
        
        return true;
    }
    catch (std::exception const& e) {
        if (chat_window_) {
            chat_window_->enqueueMessage("Error al conectar: " + std::string(e.what()));
        }
        return false;
    }
}

void WebSocketClient::runIoContext() {
    try {
        // Iniciar el bucle de recepción de mensajes
        runReceiveLoop();
        
        // Ejecutar el contexto de IO
        ioc_.run();
    }
    catch (std::exception const& e) {
        if (chat_window_) {
            chat_window_->enqueueMessage("Error en el hilo de IO: " + std::string(e.what()));
        }
        connected_ = false;
    }
}

void WebSocketClient::requestUserList() {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!connected_) return;

        // Crear mensaje de solicitud de lista de usuarios
        std::vector<uint8_t> buffer;
        buffer.push_back(LIST_USERS);  // Tipo de mensaje
        
        ws_.binary(true);
        ws_.write(net::buffer(buffer));
        
    }
    catch (std::exception const& e) {
        if (chat_window_) {
            chat_window_->enqueueMessage("Error al solicitar lista de usuarios: " + std::string(e.what()));
        }
    }
}

void WebSocketClient::requestUserInfo(const std::string& targetUser) {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!connected_) return;

        // Verificar que el nombre de usuario no exceda 255 caracteres
        if (targetUser.length() > 255) {
            if (chat_window_) {
                chat_window_->enqueueMessage("Error: Nombre de usuario demasiado largo");
            }
            return;
        }

        // Construir el mensaje: [Tipo (1 byte)] [Longitud nombre (1 byte)] [Nombre]
        std::vector<uint8_t> buffer;
        buffer.push_back(GET_USER_INFO);  // Tipo de mensaje
        buffer.push_back(static_cast<uint8_t>(targetUser.length()));  // Longitud del nombre
        
        // Añadir el nombre de usuario
        for (char c : targetUser) {
            buffer.push_back(static_cast<uint8_t>(c));
        }
        
        ws_.binary(true);
        ws_.write(net::buffer(buffer));
        
        if (chat_window_) {
            chat_window_->enqueueMessage("Solicitando información de: " + targetUser);
        }
    }
    catch (std::exception const& e) {
        if (chat_window_) {
            chat_window_->enqueueMessage("Error al solicitar información: " + std::string(e.what()));
        }
    }
}


void WebSocketClient::sendMessageToUser(const std::string& targetUser, const std::string& message) {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!connected_) return;

        // Verificar que el mensaje no exceda 255 caracteres
        if (message.length() > 255) {
            if (chat_window_) {
                chat_window_->enqueueMessage("Error: Mensaje excede 255 caracteres");
            }
            return;
        }
        
        // Verificar que el nombre del usuario no exceda 255 caracteres
        if (targetUser.length() > 255) {
            if (chat_window_) {
                chat_window_->enqueueMessage("Error: Nombre de usuario demasiado largo");
            }
            return;
        }
        
        // Construir el mensaje siguiendo el protocolo:
        // [ID (1 byte)] [Len(targetUser) (1 byte)] [targetUser] [Len(message) (1 byte)] [message]
        std::vector<uint8_t> buffer;
        buffer.push_back(SEND_MESSAGE);  // Usamos la constante SEND_MESSAGE en lugar de un literal (valor 4)
        
        // Agregar el nombre del destinatario
        buffer.push_back(static_cast<uint8_t>(targetUser.length()));
        for (char c : targetUser) {
            buffer.push_back(static_cast<uint8_t>(c));
        }
        
        // Agregar el mensaje
        buffer.push_back(static_cast<uint8_t>(message.length()));
        for (char c : message) {
            buffer.push_back(static_cast<uint8_t>(c));
        }
        
        ws_.binary(true);
        ws_.write(net::buffer(buffer));
    }
    catch (std::exception const& e) {
        if (chat_window_) {
            chat_window_->enqueueMessage("Error al enviar mensaje: " + std::string(e.what()));
        }
    }
}

void WebSocketClient::setStatus(chat::UserStatus status) {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!connected_) return;

        // Crear mensaje para cambio de estado
        std::vector<uint8_t> buffer;
        buffer.push_back(CHANGE_STATUS);     // Tipo de mensaje
        buffer.push_back(static_cast<uint8_t>(status));  // Valor del estado
        
        ws_.binary(true);
        ws_.write(net::buffer(buffer));
        
        std::string statusStr;
        switch (status) {
            case chat::ACTIVE:       statusStr = "Activo"; break;
            case chat::BUSY:         statusStr = "Ocupado"; break;
            case chat::INACTIVE:     statusStr = "Inactivo"; break;
            case chat::DISCONNECTED: statusStr = "Desconectado"; break;
            default:                 statusStr = "Activo"; break;
        }
    }
    catch (std::exception const& e) {
        if (chat_window_) {
            chat_window_->enqueueMessage("Error al cambiar estado: " + std::string(e.what()));
        }
    }
}


void WebSocketClient::runReceiveLoop() {
    try {
        while (connected_) {
            // Buffer para recibir datos
            beast::flat_buffer buffer;
            
            // Recibir mensaje
            ws_.read(buffer);
            
            // Procesar mensaje
            auto data = static_cast<const uint8_t*>(buffer.data().data());
            size_t size = buffer.data().size();
            
            if (size > 0) {
                uint8_t messageType = data[0];
                handleMessage(messageType, data + 1, size - 1);
            }
        }
    }
    catch (beast::system_error const& se) {
        if (se.code() != websocket::error::closed) {
            if (chat_window_) {
                chat_window_->enqueueMessage("Error en el bucle de recepción: " + std::string(se.code().message()));
            }
        }
        connected_ = false;
    }
    catch (std::exception const& e) {
        if (chat_window_) {
            chat_window_->enqueueMessage("Error en el bucle de recepción: " + std::string(e.what()));
        }
        connected_ = false;
    }
}

void WebSocketClient::handleMessage(uint8_t messageType, const uint8_t* data, size_t length) {
    switch(messageType) {
        case USER_LIST_RESPONSE: { // ID 51
            if (length > 0) {
                uint8_t userCount = data[0];
                std::vector<std::pair<std::string, uint8_t>> users;
                size_t offset = 1;
                for (int i = 0; i < userCount && offset < length; ++i) {
                    uint8_t usernameLength = data[offset++];
                    if (offset + usernameLength <= length) {
                        std::string username(reinterpret_cast<const char*>(data + offset), usernameLength);
                        offset += usernameLength;
                        uint8_t status = data[offset++];
                        users.push_back({username, status});
                    }
                }
                if (chat_window_) {
                    chat_window_->updateUserList(users);
                }
            }
            break;
        }
        
        case USER_INFO_RESPONSE: { // ID 52
            if (length < 2) {
                if (chat_window_) {
                    chat_window_->enqueueMessage("Error: Respuesta de información de usuario inválida");
                }
                break;
            }
        
            // Primer byte es la longitud del nombre de usuario
            uint8_t usernameLength = data[0];
            
            // Verificar que tengamos suficientes datos
            if (1 + usernameLength + 1 > length) {
                if (chat_window_) {
                    chat_window_->enqueueMessage("Error: Datos de usuario incompletos");
                }
                break;
            }
        
            // Extraer nombre de usuario
            std::string username(reinterpret_cast<const char*>(data + 1), usernameLength);
            
            // Estado del usuario (1 byte después del nombre)
            uint8_t statusValue = data[1 + usernameLength];
            chat::UserStatus status = static_cast<chat::UserStatus>(statusValue);
            
            // Resto es información adicional
            std::string info(reinterpret_cast<const char*>(data + 1 + usernameLength + 1), 
                        length - (1 + usernameLength + 1));
        
            std::string statusStr;
            switch(status) {
                case chat::ACTIVE:       statusStr = "🟢 Activo"; break;
                case chat::BUSY:         statusStr = "🔴 Ocupado"; break;
                case chat::INACTIVE:     statusStr = "🟡 Inactivo"; break;
                case chat::DISCONNECTED: statusStr = "⚪ Desconectado"; break;
                default:                 statusStr = "❓ Desconocido"; break;
            }
        
            if (chat_window_) {
                chat_window_->enqueueMessage("--- Información de " + username + " ---");
                chat_window_->enqueueMessage("Estado: " + statusStr);
                if (!info.empty()) {
                    chat_window_->enqueueMessage("Info: " + info);
                }
                chat_window_->enqueueMessage("----------------------------");
            }
            break;
        }

        case STATUS_CHANGED: { // ID 54
            if (length > 0) {
                uint8_t usernameLength = data[0];
                if (1 + usernameLength + 1 <= length) {
                    std::string username(reinterpret_cast<const char*>(data + 1), usernameLength);
                    uint8_t status = data[1 + usernameLength];
                    if (chat_window_) {
                        //chat_window_->enqueueMessage("Estado de " + username + " actualizado a: " + std::to_string(status));
                    }
                }
            }
            break;
        }        
        
        case MESSAGE_RECEIVED: { // ID 55, utilizando 1 byte para longitudes
            if (length > 0) {
                uint8_t senderLength = data[0];
                if (1 + senderLength + 1 <= length) {
                    std::string sender(reinterpret_cast<const char*>(data + 1), senderLength);
                    uint8_t messageLength = data[1 + senderLength];
                    if (1 + senderLength + 1 + messageLength <= length) {
                        std::string message(reinterpret_cast<const char*>(data + 1 + senderLength + 1), messageLength);
                        if (chat_window_) {
                            chat_window_->enqueueMessage(sender + ": " + message);
                            chat_window_->addUserToTargetList(sender);
                        }
                    }
                }
            }
            break;
        }
        
        case ERROR_MSG: { // ID 50
            std::string errorMsg(reinterpret_cast<const char*>(data), length);
            if (chat_window_) {
                chat_window_->enqueueMessage("Error: " + errorMsg);
            }
            break;
        }
        
        default:
            if (chat_window_) {
                chat_window_->enqueueMessage("Mensaje desconocido tipo: " + std::to_string(messageType));
            }
            break;
    }
}


void WebSocketClient::close() {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!connected_) return;
        
        ws_.close(websocket::close_code::normal);
        connected_ = false;
        
        if (chat_window_) {
            chat_window_->enqueueMessage("Conexión cerrada");
        }
    }
    catch (std::exception const& e) {
        if (chat_window_) {
            chat_window_->enqueueMessage("Error al cerrar conexión: " + std::string(e.what()));
        }
    }
}

// ChatWindow
ChatWindow::ChatWindow(const std::string &user, const std::string &ip, int port)
    : Fl_Window(600, 650, "ChatEZ - Conectado"), username_(user)
{
    size_range(600, 650);

    // Cabecera
    info_label = new Fl_Box(10, 10, 580, 20,
        ("Usuario: " + user + " | Servidor: " + ip + ":" + std::to_string(port)).c_str());

    // Historial (izquierda)
    chat_buf = new Fl_Text_Buffer();
    chat_disp = new Fl_Text_Display(10, 40, 400, 520);
    chat_disp->buffer(chat_buf);
    chat_disp->wrap_mode(Fl_Text_Display::WRAP_AT_BOUNDS, 0);

    // Panel derecho
    right = new Fl_Pack(420, 40, 160, 520);
    right->type(Fl_Pack::VERTICAL);
    right->spacing(5);
    
    new Fl_Box(0, 0, 160, 20, "Usuarios Conectados:");
    user_list = new Fl_Browser(0, 0, 160, 180);
    user_list->type(FL_HOLD_BROWSER);
    
    list_btn = new Fl_Button(0, 0, 160, 30, "Listar Usuarios");
    
    new Fl_Box(0, 0, 160, 20, "Consultar Usuario:");
    info_input = new Fl_Input(0, 0, 160, 25);
    info_btn = new Fl_Button(0, 0, 160, 30, "Ver Info");
    
    new Fl_Box(0, 0, 160, 20, "Estado:");
    status_choice = new Fl_Choice(0, 0, 160, 30);
    status_choice->add("Activo");
    status_choice->add("Ocupado");
    status_choice->add("Inactivo");
    status_choice->value(0);
    
    right->end();

    // Fila de mensaje
    msg_in = new Fl_Input(80, 575, 380, 25, "Mensaje:");
    target_choice = new Fl_Choice(80, 610, 380, 25, "Para:");
    target_choice->add("Chat general");
    
    send_btn = new Fl_Button(480, 575, 100, 60, "Enviar");
    send_btn->align(FL_ALIGN_CENTER);

    // Callbacks
    list_btn->callback(on_list_users, this);
    info_btn->callback(on_get_info, this);
    send_btn->callback(on_send, this);
    status_choice->callback(on_status_change, this);

    end();
    
    // Inicializar cliente WebSocket
    client_ = new WebSocketClient(user);
    client_->setChatWindow(this);
    
    // Intentar conectar
    if (!client_->connect(ip, std::to_string(port))) {
        fl_alert("Error al conectar al servidor. La aplicación se cerrará.");
        Fl::add_timeout(0.5, [](void* data) { exit(1); });
    }
    
    // Configurar el callback idle para manejar la UI y los mensajes
    Fl::add_idle(idle_cb, this);
    
    // Inicializar lista de usuarios
    Fl::add_timeout(3.0, updateUserListCallback, this);
}

void ChatWindow::on_send(Fl_Widget* w, void* data) {
    ChatWindow* cw = static_cast<ChatWindow*>(data);
    std::string msg = cw->msg_in->value();
    std::cout << "[DEBUG] Enviando mensaje: " << msg << std::endl;

    if (!msg.empty() && cw->client_->isConnected()) {
        // Si el estado actual (según la elección del usuario) es "Inactivo", pasarlo a "Activo"
        // 1 -> "Activo", 2 -> "Ocupado", 3 -> "Inactivo"
        if (cw->status_choice->value() == 2) {
            cw->status_choice->value(1); // Actualiza la UI a "Activo"
            cw->client_->setStatus(chat::ACTIVE);
        }
        
        std::string targetUser = cw->getSelectedTargetUser();
        std::cout << "[DEBUG] Destinatario seleccionado: " << targetUser << std::endl;

        cw->client_->sendMessageToUser(targetUser, msg);
        cw->appendToChat("Tú → " + targetUser + ": " + msg);
        cw->msg_in->value("");
    } else {
        std::cout << "[DEBUG] Mensaje vacío o cliente no conectado" << std::endl;
    }
}


void ChatWindow::on_list_users(Fl_Widget* w, void* data) {
    ChatWindow* cw = static_cast<ChatWindow*>(data);
    if (cw->client_->isConnected()) {
        cw->client_->requestUserList();
    }
}

void ChatWindow::on_get_info(Fl_Widget* w, void* data) {
    ChatWindow* cw = static_cast<ChatWindow*>(data);
    std::string target = cw->info_input->value();
    
    if (target.empty()) {
        cw->enqueueMessage("Error: Debes ingresar un nombre de usuario");
        return;
    }
    
    if (!cw->client_->isConnected()) {
        cw->enqueueMessage("Error: No estás conectado al servidor");
        return;
    }
    
    if (target == cw->username_) {
        cw->enqueueMessage("Info: No puedes consultar tu propia información");
        return;
    }
    
    cw->enqueueMessage("Solicitando información de: " + target);
    cw->client_->requestUserInfo(target);
}

void ChatWindow::on_status_change(Fl_Widget* w, void* data) {
    ChatWindow* cw = static_cast<ChatWindow*>(data);
    int status = cw->status_choice->value();
    
    if (cw->client_->isConnected()) {
        chat::UserStatus userStatus;
        switch (status) {
            case 0: userStatus = chat::ACTIVE; break;
            case 1: userStatus = chat::BUSY; break;
            case 2: userStatus = chat::INACTIVE; break;
            default: userStatus = chat::ACTIVE; break;
        }
    
        cw->client_->setStatus(userStatus);
        // Solicitar lista actualizada para refrescar la interfaz
        cw->client_->requestUserList();
    }
}

void ChatWindow::updateUserList(const std::vector<std::pair<std::string, uint8_t>>& users) {
    Fl::lock();
    
    // Limpiar la lista
    user_list->clear();
    
    // Agregar los usuarios con el icono y estado en paréntesis
    for (const auto& [username, status] : users) {
        std::string icon;
        std::string stateText;
        // Mapear los estados según el protocolo: 0 = Desconectado, 1 = Activo, 2 = Ocupado, 3 = Inactivo
        switch (status) {
            case chat::ACTIVE:
                icon = "■ "; 
                stateText = "(Activo)";
                break;
            case chat::BUSY:
                icon = "▲ "; 
                stateText = "(Ocupado)";
                break;
            case chat::INACTIVE:
                icon = "◆ "; 
                stateText = "(Inactivo)";
                break;
            case chat::DISCONNECTED:
                icon = "□ "; 
                stateText = "(Desconectado)";
                break;
            default:
                icon = "? ";
                stateText = "";
                break;
        }
        
        std::string displayStr = icon + username + " " + stateText;
        user_list->add(displayStr.c_str());
        
        if (username != username_) {
            addUserToTargetList(username);
        }
    }
    
    Fl::unlock();
    Fl::awake();
}

void ChatWindow::addUserToTargetList(const std::string& username) {
    if (username == username_) return; // No agregar al propio usuario

    // Verificar si ya existe
    for (int i = 0; i < target_choice->size(); i++) {
        const char* existing = target_choice->text(i);
        if (existing && username == existing) {
            return; // Ya existe
        }
    }

    // Agregar nuevo usuario
    target_choice->add(username.c_str());
}


// LoginWindow
void LoginWindow::on_connect(Fl_Widget* w, void* data) {
    LoginWindow* win = static_cast<LoginWindow*>(data);
    std::string user = win->user_in->value();
    std::string ip = win->ip_in->value();
    int port = atoi(win->port_in->value());

    if (user.empty() || ip.empty() || port <= 0) {
        fl_message("Por favor completa todos los campos correctamente.");
        return;
    }
    
    win->hide();
    
    // Crear ventana de chat
    ChatWindow* cw = new ChatWindow(user, ip, port);
    cw->show();
}

// Función principal
int main(int argc, char** argv) {
    Fl::scheme("gtk+");
    Fl::lock(); // Habilitar multithreading en FLTK
    
    LoginWindow lw;
    lw.show(argc, argv);
    
    return Fl::run();
}