#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>

int main(int argc, char* argv[]) {
    const char* ip = argc > 1 ? argv[1] : "127.0.0.1";
    int port = 8888;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    sockaddr_in serv{};
    serv.sin_family = AF_INET;
    serv.sin_port = htons(port);
    inet_pton(AF_INET, ip, &serv.sin_addr);

    if (connect(sock, (sockaddr*)&serv, sizeof(serv)) < 0) {
        perror("connect");
        return 1;
    }

    std::cout << "[Client] connected to " << ip << ":" << port << std::endl;
    std::cout
        << "[Client] input message and press Enter (type 'exit' to quit):";

    char buf[1024];
    while (true) {
        std::cout << "\n> ";
        // 1. 从终端读一行
        if (!fgets(buf, sizeof(buf), stdin))
            break;
        if (strncmp(buf, "exit", 4) == 0)
            break;

        // 2. 发给服务器
        write(sock, buf, strlen(buf));
    }

    close(sock);
    return 0;
}