#ifndef CLIENT_H
#define CLIENT_H

#include <string>
#include <thread>
#include <atomic>
#include <vector>
#include <mutex>

class Client {
private:
    int sockfd;
    std::string username;
    std::string server_ip;
    int server_port;
    std::thread recv_thread;
    std::atomic<bool> running;
    std::mutex send_mutex;

    void receive_loop(); // Loop de recepción en hilo

public:
    Client();
    ~Client();

    bool connect_to_server(const std::string& ip, int port, const std::string& user);
    void disconnect();

    void send_raw(const std::vector<uint8_t>& data);  // Envío crudo de bytes
    void handle_server_message(const std::vector<uint8_t>& data); // TODO: Implementar según protocolo
};

#endif // CLIENT_H