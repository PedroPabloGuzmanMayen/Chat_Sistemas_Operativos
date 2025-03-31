#include <algorithm>
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
#include "../client/events.h"

void refreshChatDisplay();

class ChatWindow;
ChatWindow* g_chatWindow = nullptr;

void show_chat_window(const std::string&, const std::string&, int);

class LoginWindow : public Fl_Window {
    Fl_Input *user_in, *ip_in, *port_in;
    Fl_Button *connect_btn;

    static void on_connect(Fl_Widget* w, void* data) {
        LoginWindow* win = (LoginWindow*)data;
        std::string user = win->user_in->value();
        std::string ip   = win->ip_in->value();
        int port = atoi(win->port_in->value());

        if(user.empty() || ip.empty() || port <= 0) {
            fl_message("Por favor completa todos los campos correctamente.");
            return;
        }
        if (connectToServer(ip, port, user)) {
            startNetworkService();
            win->hide();
            show_chat_window(user, ip, port);
        } else {
            fl_message("No se pudo conectar al servidor. Inténtalo de nuevo.");
        }
    }

public:
    LoginWindow(): Fl_Window(300,180,"Conectar al Chat") {
        user_in = new Fl_Input(100,20,180,25,"Usuario:");
        ip_in   = new Fl_Input(100,60,180,25,"IP Servidor:");
        ip_in->value("127.0.0.1");
        port_in = new Fl_Input(100,100,180,25,"Puerto:");
        port_in->value("9000");
        connect_btn = new Fl_Button(100,140,100,30,"Conectar");
        connect_btn->callback(on_connect,this);
        end();
    }
};

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
            cw->appendChatMessage("Tú: " + msg);
            cw->msg_in->value("");

            std::string target;
            if (cw->target_choice->value() == 0) {
                target = "~";
            } else {
                target = cw->user_list->text(cw->user_list->value());
            }

            std::vector<uint8_t> dataToSend;
            dataToSend.push_back((uint8_t)target.size());
            dataToSend.insert(dataToSend.end(), target.begin(), target.end());
            dataToSend.push_back((uint8_t)msg.size());
            dataToSend.insert(dataToSend.end(), msg.begin(), msg.end());

            sendMessageToServer(SEND_MESSAGE, dataToSend);
        }
    }

    static void on_list_users(Fl_Widget* w, void* data) {
        sendMessageToServer(USER_LIST_REQUEST, {});
    }

    static void on_info(Fl_Widget* w, void* data) {
        ChatWindow* cw = static_cast<ChatWindow*>(data);
        int selected = cw->user_list->value();
        if (selected < 1) {
            fl_message("Selecciona un usuario de la lista.");
            return;
        }
        std::string user = cw->user_list->text(selected);
        std::vector<uint8_t> payload(user.begin(), user.end());
        sendMessageToServer(GET_INFO, payload);
    }

    static void on_status_change(Fl_Widget* w, void* data) {
        ChatWindow* cw = static_cast<ChatWindow*>(data);
        int selected = cw->status_choice->value();
        std::vector<uint8_t> payload = {static_cast<uint8_t>(selected)};
        sendMessageToServer(SET_STATUS, payload);
    }

    static void on_history(Fl_Widget* w, void* data) {
        ChatWindow* cw = static_cast<ChatWindow*>(data);
        int selected = cw->user_list->value();
        if (selected < 1) {
            fl_message("Selecciona un usuario para ver historial.");
            return;
        }
        std::string user = cw->user_list->text(selected);
        std::vector<uint8_t> payload(user.begin(), user.end());
        sendMessageToServer(GET_HISTORY, payload);
    }

public:
    ChatWindow(const std::string &user, const std::string &ip, int port)
    : Fl_Window(600,650,"ChatEZ") {
        size_range(600,650);

        info_label = new Fl_Box(10,10,580,20,
            ("Usuario: " + user + " | Servidor: " + ip + ":" + std::to_string(port)).c_str());

        Fl_Group* top = new Fl_Group(0, 0, 600, 610);
            chat_buf = new Fl_Text_Buffer;
            chat_disp = new Fl_Text_Display(10,40,400,280);
            chat_disp->buffer(chat_buf);

            right = new Fl_Pack(420,40,160,280);
            right->type(Fl_Pack::VERTICAL);
            right->spacing(5);
                new Fl_Box(0,0,160,20,"Usuarios Conectados:");
                user_list = new Fl_Browser(0,0,160,100);
                list_btn = new Fl_Button(0,0,160,30,"Listar Usuarios");
                info_input = new Fl_Input(0,0,160,25);
                info_btn = new Fl_Button(0,0,160,30,"Ver Info");
                Fl_Button* history_btn = new Fl_Button(0,0,160,30,"Ver Historial");
                new Fl_Box(0,0,160,20,"Estado:");
                status_choice = new Fl_Choice(0,0,160,30);
                status_choice->add("Activo|Ocupado|Inactivo");
                status_choice->value(0);
            right->end();
        top->end();
        resizable(top);

        Fl_Group* bottom = new Fl_Group(0, 610, 600, 40);
            target_choice = new Fl_Choice(10,615,150,25,"Para:");
            target_choice->add("Chat general");
            msg_in = new Fl_Input(170,615,300,25);
            send_btn = new Fl_Button(480,615,110,25,"Enviar");
            send_btn->align(FL_ALIGN_CENTER);
        bottom->end();

        send_btn->callback(on_send, this);
        list_btn->callback(on_list_users, this);
        info_btn->callback(on_info, this);
        history_btn->callback(on_history, this);
        status_choice->callback(on_status_change, this);

        end();
    }

    void appendChatMessage(const std::string &msg) {
        chat_buf->append((msg + "\n").c_str());
    }

    void updateUserList(const std::vector<std::string>& users) {
        user_list->clear();
        target_choice->clear();
        target_choice->add("Chat general");

        for (const std::string& name : users) {
            user_list->add(name.c_str());
            target_choice->add(name.c_str());
        }
    }
};

void show_chat_window(const std::string &user, const std::string &ip, int port) {
    ChatWindow *cw = new ChatWindow(user, ip, port);
    g_chatWindow = cw;
    cw->show();
}

void refreshChatDisplay() {
    if (g_chatWindow && !chatHistory.empty()) {
        g_chatWindow->appendChatMessage(chatHistory.back());
        g_chatWindow->updateUserList(userList);
    }
}

int main(int argc, char **argv) {
    Fl::scheme("plastic");
    LoginWindow lw;
    lw.show(argc,argv);
    return Fl::run();
}