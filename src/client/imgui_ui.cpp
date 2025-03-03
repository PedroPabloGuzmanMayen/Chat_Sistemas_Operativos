#include "imgui_ui.h"
#include "network.h"
#include "third_party/imgui/imgui.h"
#include "third_party/imgui/imgui_impl_glfw.h"
#include "third_party/imgui/imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <iostream>

GLFWwindow* window;

/**
 * Renderiza la UI con ImGui
 */
void renderUI() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Chat Cliente");

    // Mostrar usuarios conectados
    ImGui::Text("Usuarios Conectados:");
    for (const std::string &user : userList) {
        ImGui::BulletText("%s", user.c_str());
    }

    // Mostrar historial de chat
    ImGui::Text("Historial:");
    for (const std::string &msg : chatHistory) {
        ImGui::TextWrapped("%s", msg.c_str());
    }

    // Mostrar errores
    if (!errorMessage.empty()) {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "%s", errorMessage.c_str());
    }

    // Campo de entrada para enviar mensajes
    static char messageBuffer[255] = "";
    ImGui::InputText("Mensaje", messageBuffer, IM_ARRAYSIZE(messageBuffer));

    if (ImGui::Button("Enviar")) {
        std::string msg = messageBuffer;
        std::vector<uint8_t> data(msg.begin(), msg.end());
        sendMessageToServer(4, data);
    }

    ImGui::End();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}