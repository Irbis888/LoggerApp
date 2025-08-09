#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <algorithm>
#include <cstring>
#include <unistd.h>
#include <deque>
#include <netinet/in.h>
#include <sys/socket.h>

const int PORT = 9999;
const int MAX_BUFFER = 1024;

std::vector<int> client_fds;
std::mutex clients_mutex;

//Retranslation tool for stat collecting
void broadcastToOthers(int sender_fd, const std::string& message) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    for (int fd : client_fds) {
        if (fd != sender_fd) {
            send(fd, message.c_str(), message.size(), 0);
        }
    }
}

void handleClient(int client_fd) {
    char buffer[MAX_BUFFER];

    while (true) {
        ssize_t bytesRead = read(client_fd, buffer, MAX_BUFFER - 1);
        if (bytesRead <= 0) {
            break;
        }

        buffer[bytesRead] = '\0';
        std::string message(buffer);

        // Write on server
        std::cout << "Log: " << message;

        // Broadcast to everyone else
        broadcastToOthers(client_fd, message);
    }

    // Close and delete a client
    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        client_fds.erase(std::remove(client_fds.begin(), client_fds.end(), client_fd), client_fds.end());
    }

    close(client_fd);
    std::cout << "Client disconnected.\n";
}

int main() {
    int server_fd;
    sockaddr_in server_addr{};
    int opt = 1;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        return 1;
    }

    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;  // 0.0.0.0
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        return 1;
    }

    if (listen(server_fd, 10) < 0) {
        perror("listen");
        return 1;
    }

    std::cout << "Server runs at " << PORT << "\n";

    while (true) {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);

        int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }

        std::cout << "New client connected!\n";

        {
            std::lock_guard<std::mutex> lock(clients_mutex);
            client_fds.push_back(client_fd);
        }

        std::thread(handleClient, client_fd).detach();
    }

    close(server_fd);
    return 0;
}
