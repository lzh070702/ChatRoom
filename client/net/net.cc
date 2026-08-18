#include "net.h"

#include <readline/readline.h>
#include <sys/epoll.h>
#include <unistd.h>

#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <iostream>

#include "TcpClient.h"

void netLoop(TcpClient* client) {
    int epfd = epoll_create1(0);
    epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.fd = client->getFd();
    epoll_ctl(epfd, EPOLL_CTL_ADD, client->getFd(), &ev);
    auto last_send = std::chrono::steady_clock::now();
    auto last_recv = std::chrono::steady_clock::now();
    epoll_event events[4];
    while (g_running) {
        int n = epoll_wait(epfd, events, 4, 500);
        auto now = std::chrono::steady_clock::now();
        if (now - last_send >= std::chrono::seconds(20)) {
            client->sendData(R"({"type":0})");
            last_send = now;
        }
        if (now - last_recv >= std::chrono::seconds(60)) {
            pushOpt(RED + std::string("与服务器断开连接") + RESET);
            g_running = false;
            g_ipt_cv.notify_all();
            g_rsp_cv.notify_all();
            break;
        }
        client->flush();
        for (int i = 0; i < n; ++i) {
            if (events[i].data.fd != client->getFd()) {
                continue;
            }
            std::string msg = client->recvData();
            while (!msg.empty()) {
                parseOpt(msg, last_recv);
                msg = client->recvData();
            }
            if (client->isClosed()) {
                pushOpt(RED + std::string("与服务器断开连接") + RESET);
                g_running = false;
                g_ipt_cv.notify_all();
                g_rsp_cv.notify_all();
                break;
            }
        }
    }
    close(epfd);
}

void ioLoop() {
    int epfd = epoll_create1(0);
    epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.fd = STDIN_FILENO;
    epoll_ctl(epfd, EPOLL_CTL_ADD, STDIN_FILENO, &ev);
    ev.data.fd = g_opt_efd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, g_opt_efd, &ev);
    rl_callback_handler_install("", on_line);
    epoll_event events[2];
    while (g_running) {
        int n = epoll_wait(epfd, events, 2, 100);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            break;
        }
        for (int i = 0; i < n; i++) {
            if (events[i].data.fd == g_opt_efd) {
                uint64_t val;
                read(g_opt_efd, &val, sizeof(val));
                printOpt();
            } else if (events[i].data.fd == STDIN_FILENO) {
                rl_callback_read_char();
            }
        }
    }
    rl_callback_handler_remove();
    close(epfd);
}

void on_line(char* line) {
    if (line == nullptr) {
        g_running = false;
        g_ipt_cv.notify_all();
        g_rsp_cv.notify_all();
        return;
    }
    std::string prefix;
    {
        std::lock_guard<std::mutex> lk(g_ui_mutex);
        prefix = g_prefix;
    }
    if (*line) {
        std::string l(line);
        for (int i = 0; i < l.size() / 80; i++) {
            std::cout << "\033[A\033[K";
        }
        fprintf(rl_outstream, "\033[A\033[K\033[A\033[K%s %s\n", prefix.c_str(),
                line);
        pushIpt(line);
    } else {
        fprintf(rl_outstream, "\033[A");
    }
    free(line);
    rl_callback_handler_remove();
    rl_callback_handler_install("", on_line);
}

void parseOpt(const std::string& msg,
              std::chrono::steady_clock::time_point& last_recv) {
    json js;
    try {
        js = json::parse(msg);
    } catch (...) {
        return;
    }
    int type = js["type"];
    if (type == 0) {
        last_recv = std::chrono::steady_clock::now();
        return;
    }
    int code = js["code"];
    if (code == 2) {
        std::string rsp;
        if (type == 9) {
            rsp = std::string(js["name"]) + std::string(js["msg"]);
        }
        if (type == 13) {
            bool is_file = js["msg_type"];
            if (isCurrentSession(0, js["id"])) {
                rsp = "\033[0m" + std::string(js["name"]) + ": " +
                      (is_file ? "[文件] " + std::string(js["msg"])
                               : std::string(js["msg"]));
            } else {
                rsp = is_file
                          ? "好友" + std::string(js["name"]) + "发来一个文件"
                          : "好友" + std::string(js["name"]) + "发来一条消息";
            }
        }
        if (type == 17) {
            rsp = std::string(js["msg"]) + std::string(js["group_name"]);
        }
        if (type == 18) {
            rsp = std::string(js["msg"]) + std::string(js["name"]);
        }
        if (type == 19) {
            rsp = std::string(js["msg"]);
        }
        if (type == 25) {
            bool is_file = js["msg_type"];
            std::string sender = js["sender_name"];
            if (isCurrentSession(1, js["group_id"])) {
                rsp = "\033[0m" + sender + ": " +
                      (is_file ? sender + ": [文件] " + std::string(js["msg"])
                               : sender + ": " + std::string(js["msg"]));
            } else {
                rsp = is_file
                          ? sender + " 在群聊" + std::string(js["group_name"]) +
                                "发来一个文件"
                          : sender + " 在群聊" + std::string(js["group_name"]) +
                                "发来一条消息";
            }
        }
        std::string prompt;
        {
            std::lock_guard<std::mutex> lk(g_ui_mutex);
            prompt = g_prompt;
        }
        pushOpt(CYAN + std::string("\033[A\033[K") + rsp + std::string("\n") +
                RESET + prompt);
        return;
    }
    pushRsp(js);
}