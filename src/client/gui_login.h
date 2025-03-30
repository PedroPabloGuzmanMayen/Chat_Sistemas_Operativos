#ifndef GUI_LOGIN_H
#define GUI_LOGIN_H

#include <FL/Fl_Window.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <FL/fl_ask.H>
#include <string>

// Declaración de función para abrir la ventana principal del chat
void show_chat_window(const std::string& user, const std::string& ip, int port);

class LoginWindow : public Fl_Window {
private:
    Fl_Input *user_in, *ip_in, *port_in;
    Fl_Button *connect_btn;

    static void on_connect(Fl_Widget* w, void* data) {
        LoginWindow* win = static_cast<LoginWindow*>(data);
        std::string user = win->user_in->value();
        std::string ip   = win->ip_in->value();
        int port = atoi(win->port_in->value());

        if (user.empty() || ip.empty() || port <= 0) {
            fl_message("Por favor completa todos los campos correctamente.");
            return;
        }

        win->hide();
        show_chat_window(user, ip, port);
    }

public:
    LoginWindow() : Fl_Window(300, 180, "Conectar al Chat") {
        user_in = new Fl_Input(100, 20, 180, 25, "Usuario:");
        ip_in   = new Fl_Input(100, 60, 180, 25, "IP Servidor:");
        ip_in->value("3.22.206.194");
        port_in = new Fl_Input(100, 100, 180, 25, "Puerto:");
        port_in->value("8080");
        connect_btn = new Fl_Button(100, 140, 100, 30, "Conectar");
        connect_btn->callback(on_connect, this);
        end();
    }
};

#endif // GUI_LOGIN_H