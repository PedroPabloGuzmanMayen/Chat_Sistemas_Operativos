// src/client/client.cpp

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Text_Buffer.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Browser.H>
#include <string>
#include <FL/Fl_Box.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Pack.H>
#include "network.h"
#include "events.h"

// Forward declaration
void show_chat_window(const std::string&, const std::string&, int);

//
// ==== LOGIN WINDOW ====
//
class LoginWindow : public Fl_Window {
    Fl_Input *user_in, *ip_in, *port_in;
    Fl_Button *connect_btn;

    static void on_connect(Fl_Widget* w, void* data) {
        // Obtenemos la ventana de login desde el puntero de datos
        LoginWindow* win = (LoginWindow*)data;
        
        // Extraemos usuario, IP y puerto desde los campos de la UI
        std::string user = win->user_in->value();
        std::string ip   = win->ip_in->value();
        int port = atoi(win->port_in->value());
        
        // Validamos que los campos no estén vacíos y que el puerto sea válido
        if (user.empty() || ip.empty() || port <= 0) {
            fl_message("Por favor completa todos los campos correctamente.");
            return;
        }
        
        // Intentamos conectar al servidor pasando la IP, el puerto y el usuario.
        // Si la conexión es exitosa, se procede a iniciar el servicio de red y mostrar la ventana de chat.
        if (connectToServer(ip, port, user)) {
            // Iniciamos el loop de mensajes en un hilo (por ejemplo, startNetworkService)
            startNetworkService();
            
            // Ocultamos la ventana de login y mostramos la ventana de chat
            win->hide();
            show_chat_window(user, ip, port);
        } else {
            // Si la conexión falla, mostramos un mensaje de error al usuario
            fl_message("No se pudo conectar al servidor. Inténtalo de nuevo.");
        }
    }
    

public:
    LoginWindow(): Fl_Window(300,180,"Conectar al Chat") {
        user_in = new Fl_Input(100,20,180,25,"Usuario:");
        ip_in   = new Fl_Input(100,60,180,25,"IP Servidor:");
        ip_in->value("3.22.206.194");
        port_in = new Fl_Input(100,100,180,25,"Puerto:");
        port_in->value("9000");
        connect_btn = new Fl_Button(100,140,100,30,"Conectar");
        connect_btn->callback(on_connect,this);
        end();
    }
};

//
// ==== CHAT WINDOW ====
//
class ChatWindow : public Fl_Window {
    Fl_Text_Display *chat_disp;
    Fl_Text_Buffer *chat_buf;
    Fl_Input *msg_in;
    Fl_Button *send_btn;
    Fl_Choice *status_choice;
    Fl_Browser *user_list;
    Fl_Box *info_label;
    Fl_Button *list_btn, *info_btn;
    Fl_Input  *info_input;
    Fl_Choice *target_choice;
    Fl_Pack   *right;
    Fl_Group *top, *bottom;

    static void on_send(Fl_Widget* w, void* data) {
        ChatWindow* cw = (ChatWindow*)data;
        std::string msg = cw->msg_in->value();
        if (!msg.empty()) {
            cw->chat_buf->append(("Tú: " + msg + "\n").c_str());
            cw->msg_in->value("");
    
            // Determinar el destinatario: si es "Chat general", usamos "~"
            std::string target;
            if (cw->target_choice->value() == 0) { 
                target = "~"; 
            } else {
                target = cw->user_list->text(cw->user_list->value());
            }
            
            // Empaquetamos el mensaje:
            // [Len Target][Target][Len Mensaje][Mensaje]
            std::vector<uint8_t> dataToSend;
            uint8_t targetLen = target.size();
            dataToSend.push_back(targetLen);
            dataToSend.insert(dataToSend.end(), target.begin(), target.end());
            uint8_t msgLen = msg.size();
            dataToSend.push_back(msgLen);
            dataToSend.insert(dataToSend.end(), msg.begin(), msg.end());
            
            // Enviamos con el tipo de mensaje SEND_MESSAGE (definido en events.h, por ejemplo 4)
            sendMessageToServer(SEND_MESSAGE, dataToSend);
        }
    }    

public:
    ChatWindow(const std::string &user, const std::string &ip, int port)
    : Fl_Window(600,650,"ChatEZ") {
        size_range(600,650);

        info_label = new Fl_Box(10,10,580,20,
            ("Usuario: " + user + " | Servidor: " + ip + ":" + std::to_string(port)).c_str());

        // TOP GROUP
        top = new Fl_Group(0, 0, w(), h() - 40);
            chat_buf = new Fl_Text_Buffer;
            chat_disp = new Fl_Text_Display(10,40,400,280);
            chat_disp->buffer(chat_buf);

            right = new Fl_Pack(420,40,160,280);
            right->type(Fl_Pack::VERTICAL);
            right->spacing(5);
            new Fl_Box(0,0,160,20,"Usuarios Conectados:");
            user_list   = new Fl_Browser(0,0,160,120);
            list_btn    = new Fl_Button(0,0,160,30,"Listar Usuarios");
            info_input  = new Fl_Input(0,0,160,25);
            info_btn    = new Fl_Button(0,0,160,30,"Ver Info");
            new Fl_Box(0,0,160,20,"Estado:");
            status_choice = new Fl_Choice(0,0,160,30);
            status_choice->add("Activo|Ocupado|Inactivo");
            status_choice->value(0);
            right->end();
        top->end();
        resizable(top);

        // BOTTOM GROUP
        bottom = new Fl_Group(0, h() - 40, w(), 40);
            target_choice = new Fl_Choice(10,h() - 35,150,25,"Para:");
            target_choice->add("Chat general");
            msg_in = new Fl_Input(170,h() - 35,300,25);
            send_btn = new Fl_Button(480,h() - 35,110,25,"Enviar");
            send_btn->align(FL_ALIGN_CENTER);
        bottom->end();

        list_btn->callback([](Fl_Widget*, void*){});
        info_btn->callback([](Fl_Widget*, void*){});
        send_btn->callback(on_send,this);

        end();
    }

    // Override resize so bottom stays anchored
    void resize(int X, int Y, int W, int H) override {
        Fl_Window::resize(X,Y,W,H);
        top->resize(0,0,W,H - 40);
        bottom->resize(0,H - 40,W,40);
    }
};


void show_chat_window(const std::string &user, const std::string &ip, int port) {
    ChatWindow *cw = new ChatWindow(user, ip, port);
    cw->show();
}

int main(int argc, char **argv) {
    Fl::scheme("plastic");
    LoginWindow lw;
    lw.show(argc,argv);
    return Fl::run();
}