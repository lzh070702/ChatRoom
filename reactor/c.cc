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
        << "[Client] input message and press Enter (type 'exit' to quit):\n> ";

    char buf[1024];
    while (true) {
        // 1. 从终端读一行
        if (!fgets(buf, sizeof(buf), stdin))
            break;
        if (strncmp(buf, "exit", 4) == 0)
            break;

        // 2. 发给服务器
        write(sock, buf, strlen(buf));

        // 3. 等服务器回显（原样发回）
        ssize_t n = read(sock, buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';
            std::cout << "[Echo] " << buf << "> ";
        } else {
            std::cout << "[Client] server closed connection." << std::endl;
            break;
        }
    }

    close(sock);
    return 0;
}