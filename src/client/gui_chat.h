#ifndef GUI_CHAT_H
#define GUI_CHAT_H

#include <FL/Fl_Window.H>
#include <string>

// Forward declarations
class Client;

// Declaración para que LoginWindow pueda llamarla
void show_chat_window(const std::string &user, const std::string &ip, int port);

#endif // GUI_CHAT_H