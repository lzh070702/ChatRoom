#include <arpa/inet.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <termios.h>
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

    // 保存原始终端属性，设为非规范模式（关闭行缓冲和回显）
    struct termios orig_term;
    tcgetattr(STDIN_FILENO, &orig_term);
    struct termios raw = orig_term;
    raw.c_iflag &= ~(ICRNL);         // 禁止将 \r 转为 \n
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_cc[VMIN] = 0;   // read 非阻塞
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

    std::cout << "[Client] connected to " << ip << ":" << port << std::endl;
    std::cout << "[Client] type 'exit' to quit" << std::endl;
    std::cout << "> " << std::flush;

    // 创建 epoll，只监听 socket
    int epfd = epoll_create1(0);
    if (epfd < 0) {
        perror("epoll_create1");
        return 1;
    }

    epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.fd = sock;
    epoll_ctl(epfd, EPOLL_CTL_ADD, sock, &ev);

    char input_buf[1024];  // 用户输入缓冲区
    int  input_len = 0;    // 缓冲区中已输入的字符数
    bool running = true;

    while (running) {
        // 1. 非阻塞逐字节读取终端输入
        char c;
        while (read(STDIN_FILENO, &c, 1) > 0) {
            if (c == '\n') continue;       // 忽略 LF
            if (c == '\r') {               // CR = 回车
                std::cout << "\r\n" << std::flush;
                if (input_len > 0) {
                    if (strncmp(input_buf, "exit", 4) == 0 &&
                        input_len == 4) {
                        running = false;
                        break;
                    }
                    write(sock, input_buf, input_len);
                    input_len = 0;
                }
                std::cout << "> " << std::flush;
            } else if (c == 127 || c == '\b') {  // 退格
                if (input_len > 0) {
                    input_len--;
                    std::cout << "\b \b" << std::flush;
                }
            } else if (input_len < (int)sizeof(input_buf) - 1) {
                input_buf[input_len++] = c;
                std::cout << c << std::flush;
            }
        }
        if (!running) break;

        // 2. epoll 等待 socket 数据（50ms 超时）
        epoll_event events[1];
        int nfds = epoll_wait(epfd, events, 1, 50);
        for (int i = 0; i < nfds; ++i) {
            if (events[i].data.fd == sock) {
                char rbuf[1024];
                ssize_t n = recv(sock, rbuf, sizeof(rbuf) - 1, 0);
                if (n > 0) {
                    rbuf[n] = '\0';
                    // 清除当前行（光标回到行首，擦除整行）
                    std::cout << "\r\033[2K";
                    // 打印服务器消息
                    std::cout << "[Server] " << rbuf;
                    // 恢复提示符和之前已输入的内容
                    std::cout << "\n> ";
                    if (input_len > 0)
                        std::cout.write(input_buf, input_len);
                    std::cout << std::flush;
                } else if (n == 0) {
                    std::cout << "\r\033[2K[Client] server closed connection"
                              << std::endl;
                    running = false;
                    break;
                } else {
                    perror("recv");
                }
            }
        }
    }

    // 恢复终端属性
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_term);
    close(epfd);
    close(sock);
    return 0;
}
