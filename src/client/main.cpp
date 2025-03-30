#include <FL/Fl.H>
#include "gui_login.h"  // ventana de login

int main(int argc, char **argv) {
    Fl::scheme("plastic");  // Estética de FLTK
    LoginWindow login;
    login.show(argc, argv);
    return Fl::run();
}