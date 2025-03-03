#include "network.h"
#include "imgui_ui.h"

/**
 * Punto de entrada del cliente.
 */
int main() {
    if (!connectToServer("127.0.0.1", 8080)) {
        return -1;
    }

    startUI();
    return 0;
}