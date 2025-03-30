#include "gui_chat.h"
#include "client.h"
#include "protocol.h"

#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Text_Buffer.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Browser.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Pack.H>
#include <FL/fl_ask.H>

class ChatWindowImpl : public Fl_Window {
private:
    Client* client;
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

    static void on_send(Fl_Widget* w, void* data) {
        auto* cw = static_cast<ChatWindowImpl*>(data);
        std::string msg = cw->msg_in->value();
        if (!msg.empty()) {
            cw->chat_buf->append(("Tú: " + msg + "\n").c_str());
            cw->msg_in->value("");

            const char* to_raw = cw->target_choice->text();
            std::string to = (!to_raw || std::string(to_raw) == "Chat general") ? "~" : to_raw;

            auto bin = Protocol::make_send_message(to, msg);
            cw->client->send_raw(bin);
        }
    }

public:
    ChatWindowImpl(Client* clientInstance, const std::string &user, const std::string &ip, int port)
    : Fl_Window(600, 650, "ChatEZ"), client(clientInstance) {
        size_range(600, 650);

        info_label = new Fl_Box(10, 10, 580, 20,
            ("Usuario: " + user + " | Servidor: " + ip + ":" + std::to_string(port)).c_str());

        Fl_Group* top = new Fl_Group(0, 0, 600, 610);
            chat_buf = new Fl_Text_Buffer;
            chat_disp = new Fl_Text_Display(10, 40, 400, 280);
            chat_disp->buffer(chat_buf);

            right = new Fl_Pack(420, 40, 160, 280);
            right->type(Fl_Pack::VERTICAL);
            right->spacing(5);

                new Fl_Box(0, 0, 160, 20, "Usuarios Conectados:");
                user_list = new Fl_Browser(0, 0, 160, 120);
                list_btn = new Fl_Button(0, 0, 160, 30, "Listar Usuarios");
                info_input = new Fl_Input(0, 0, 160, 25);
                info_btn = new Fl_Button(0, 0, 160, 30, "Ver Info");
                new Fl_Box(0, 0, 160, 20, "Estado:");
                status_choice = new Fl_Choice(0, 0, 160, 30);
                status_choice->add("Activo|Ocupado|Inactivo");
                status_choice->value(0);

            right->end();
        top->end();
        resizable(top);

        Fl_Group* bottom = new Fl_Group(0, 610, 600, 40);
            target_choice = new Fl_Choice(10, 615, 150, 25, "Para:");
            target_choice->add("Chat general");
            msg_in = new Fl_Input(170, 615, 300, 25);
            send_btn = new Fl_Button(480, 615, 110, 25, "Enviar");
            send_btn->align(FL_ALIGN_CENTER);
        bottom->end();

        list_btn->callback([](Fl_Widget*, void*){});
        info_btn->callback([](Fl_Widget*, void*){});
        send_btn->callback(on_send, this);

        end();
    }
};

void show_chat_window(const std::string &user, const std::string &ip, int port) {
    Client* client = new Client();
    if (!client->connect_to_server(ip, port, user)) {
        fl_alert("No se pudo conectar al servidor.");
        delete client;
        return;
    }

    ChatWindowImpl* cw = new ChatWindowImpl(client, user, ip, port);
    cw->show();
}