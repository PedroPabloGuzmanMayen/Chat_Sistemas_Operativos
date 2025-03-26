#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Button.H>

void on_button_click(Fl_Widget*, void*) {
    printf("¡Botón presionado!\n");
}

int main() {
    Fl_Window window(300, 200, "FLTK Ejemplo");
    Fl_Button button(100, 75, 100, 50, "Presiona");
    button.callback(on_button_click);
    
    window.show();
    return Fl::run();
}