// simple_server.cpp — исправленная версия
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string>
#include <thread>
#include <vector>
#include <atomic>

int main() {
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;
    socklen_t addrlen = sizeof(address);

    // Создаём сокет
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Настройка сокета
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(9999);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Увеличиваем backlog
    if (listen(server_fd, 5) < 0) {  // было 1, теперь 5
        perror("listen");
        exit(EXIT_FAILURE);
    }

    std::cout << "Сервер слушает на порту 9999...\n";

    std::vector<std::thread> clients;

    while (true) {
        int client_fd = accept(server_fd, (struct sockaddr*)&address, &addrlen);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }

        std::cout << "Клиент подключён!\n";

        //  Обрабатываем клиента в отдельном потоке
        clients.push_back(std::thread([client_fd]() {
            char buffer[1024];
            while (true) {
                int n = read(client_fd, buffer, sizeof(buffer) - 1);
                if (n <= 0) break;
                buffer[n] = '\0';
                std::cout << "Лог: " << buffer;
            }
            close(client_fd);
            std::cout << "Клиент отключён.\n";
        }));
    }

    // Очистка (не достигается, но для порядка)
    for (auto& t : clients) {
        if (t.joinable()) t.join();
    }

    close(server_fd);
    return 0;
}