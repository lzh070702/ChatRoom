#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <unistd.h>
#include <algorithm>
#include <atomic>
#include <cstring>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

using namespace std;

const int PORT = 8888;
const int SUB_CNT = 4;
const int MAX_EVENTS = 1024;

struct SubReactor {
    int epfd;
    int wakeup_fd;
    atomic<int> conn_count{0};
};

mutex mtx;
vector<int> fds;

void set_nonblock(int fd);
vector<SubReactor*> subs(SUB_CNT);

void subReactorLoop(int idx);

int main() {
    //  初始化 Sub Reactor
    for (int i = 0; i < SUB_CNT; ++i) {
        auto* sub = new SubReactor{};
        sub->epfd = epoll_create1(0);
        sub->wakeup_fd = eventfd(0, EFD_NONBLOCK);
        epoll_event ev{};
        ev.events = EPOLLIN | EPOLLET;
        ev.data.fd = sub->wakeup_fd;
        epoll_ctl(sub->epfd, EPOLL_CTL_ADD, sub->wakeup_fd, &ev);
        subs[i] = sub;

        thread(subReactorLoop, i).detach();
    }

    //  创建监听 socket
    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    set_nonblock(lfd);
    int opt = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    sockaddr_in addr{AF_INET, htons(PORT), INADDR_ANY};
    bind(lfd, (sockaddr*)&addr, sizeof(addr));
    listen(lfd, 1024);
    cout << "[Main] listening on port " << PORT << ", " << SUB_CNT
         << " sub reactors ready." << endl;

    // Main 自己的 epoll，只挂 lfd
    int main_epfd = epoll_create1(0);
    epoll_event ev{}, events[MAX_EVENTS];
    ev.events = EPOLLIN;
    ev.data.fd = lfd;
    epoll_ctl(main_epfd, EPOLL_CTL_ADD, lfd, &ev);

    while (true) {
        int n = epoll_wait(main_epfd, events, MAX_EVENTS, -1);
        for (int i = 0; i < n; ++i) {
            if (events[i].data.fd == lfd) {
                while (true) {
                    sockaddr_in cli{};
                    socklen_t len = sizeof(cli);
                    int cfd = accept(lfd, (sockaddr*)&cli, &len);
                    if (cfd < 0) {
                        break;
                    }
                    int min_idx{0};
                    for (int j = 1; j < SUB_CNT; ++j) {
                        if (subs[j]->conn_count < subs[min_idx]->conn_count)
                            min_idx = j;
                    }
                    {
                        unique_lock<mutex> lock(mtx);
                        fds.push_back(cfd);
                    }
                    uint64_t u = 1;
                    write(subs[min_idx]->wakeup_fd, &u, sizeof(u));
                }
            }
        }
    }

    close(lfd);
    close(main_epfd);
    return 0;
}

void set_nonblock(int fd) {
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
}

void subReactorLoop(int idx) {
    SubReactor* self = subs[idx];
    int epfd = self->epfd;
    epoll_event ev, events[MAX_EVENTS];

    while (true) {
        int n = epoll_wait(epfd, events, MAX_EVENTS, -1);
        for (int i = 0; i < n; ++i) {
            int fd = events[i].data.fd;
            //  1. 主线程派发新连接
            if (fd == self->wakeup_fd) {
                uint64_t tmp;
                read(fd, &tmp, sizeof(tmp));
                vector<int> cfds;
                {
                    unique_lock<mutex> lock(mtx);
                    cfds.swap(fds);
                }
                for (const auto& fd : cfds) {
                    set_nonblock(fd);
                    ev.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
                    ev.data.fd = fd;
                    if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) == 0) {
                        self->conn_count++;
                    } else {
                        close(fd);
                    }
                    cout << "[Sub-" << idx << "] register fd=" << fd
                         << " (total=" << self->conn_count.load() << ")"
                         << endl;
                }
            }
            // 2. 断开或出错
            else if (events[i].events & (EPOLLRDHUP | EPOLLERR | EPOLLHUP)) {
                cout << "[Sub-" << idx << "] fd=" << fd << " closed" << endl;
                epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
                close(fd);
                self->conn_count--;
            }
            // 3. 读数据 -> 原样发回
            else if (events[i].events & EPOLLIN) {
                char buf[4096];
                while (true) {
                    ssize_t nread = read(fd, buf, sizeof(buf));
                    if (nread > 0) {
                        // write(fd, buf, nread); // 后续代码逻辑
                    } else if (nread == 0 || (nread < 0 && errno != EAGAIN)) {
                        epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
                        close(fd);
                        self->conn_count--;
                        break;
                    } else {
                        break;
                    }
                }
            }
        }
    }
}