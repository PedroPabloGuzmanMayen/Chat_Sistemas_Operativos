#include "client.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

Client::Client() : sockfd(-1), running(false) {}

Client::~Client() {
    disconnect();
}

bool Client::connect_to_server(const std::string& ip, int port, const std::string& user) {
    server_ip = ip;
    server_port = port;
    username = user;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        std::cerr << "Error al crear socket\n";
        return false;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr) <= 0) {
        std::cerr << "IP inválida\n";
        return false;
    }

    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "No se pudo conectar con el servidor\n";
        return false;
    }

    running = true;
    recv_thread = std::thread(&Client::receive_loop, this);
    return true;
}

void Client::disconnect() {
    running = false;
    if (recv_thread.joinable()) {
        recv_thread.join();
    }
    if (sockfd != -1) {
        close(sockfd);
        sockfd = -1;
    }
}

void Client::send_raw(const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(send_mutex);
    send(sockfd, data.data(), data.size(), 0);
}

void Client::receive_loop() {
    uint8_t buffer[1024];
    while (running) {
        ssize_t bytes = recv(sockfd, buffer, sizeof(buffer), 0);
        if (bytes <= 0) {
            std::cerr << "Desconectado del servidor.\n";
            running = false;
            break;
        }
        std::vector<uint8_t> data(buffer, buffer + bytes);
        handle_server_message(data);
    }
}

void Client::handle_server_message(const std::vector<uint8_t>& data) {
    // Aquí se parseará el mensaje usando protocol.h
    std::cout << "[DEBUG] Mensaje recibido de tamaño: " << data.size() << "\n";
}