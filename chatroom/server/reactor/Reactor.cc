#include "Reactor.h"
#include <fcntl.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <iostream>  ////////////////////////////

Reactor::Reactor() {
    m_epfd = epoll_create1(0);
    m_wakeup_fd = eventfd(0, EFD_NONBLOCK);
    epoll_event ev{};
    ev.events = EPOLLIN | EPOLLET;
    ev.data.ptr = &m_wakeup_fd;
    epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_wakeup_fd, &ev);
}

Reactor ::~Reactor() {
    close(m_wakeup_fd);
    close(m_epfd);
}

void Reactor ::loop() {
    epoll_event events[MAX_EVENTS];
    while (true) {
        int n = epoll_wait(m_epfd, events, MAX_EVENTS, -1);
        for (int i = 0; i < n; i++) {
            if (events[i].data.ptr == &m_wakeup_fd) {
                handleWakeup();
            } else {
                Connection* conn = static_cast<Connection*>(events[i].data.ptr);
                int fd = conn->getFd();
                if (events[i].events & (EPOLLRDHUP | EPOLLERR | EPOLLHUP)) {
                    handleClose(conn);
                } else if (events[i].events & EPOLLIN) {
                    handleRead(conn);
                }
            }
        }
    }
}

void Reactor ::pushFd(int fd) {
    {
        std::unique_lock<std::mutex> lock(m_mtx);
        m_cds.push_back(fd);
    }
    uint64_t u = 1;
    write(m_wakeup_fd, &u, sizeof(u));
}

int Reactor ::getWakeupFd() const {
    return m_wakeup_fd;
}

int Reactor ::getConnCnt() const {
    return m_conn_cnt;
}

void Reactor ::handleWakeup() {
    uint64_t tmp;
    while (read(m_wakeup_fd, &tmp, sizeof(tmp)) > 0)
        ;
    epoll_event ev{};
    std::vector<int> cfds;
    {
        std::unique_lock<std::mutex> lock(m_mtx);
        cfds.swap(m_cds);
    }
    for (const auto& fd : cfds) {
        fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
        ev.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
        Connection* conn = new Connection(fd);
        ev.data.ptr = conn;
        if (epoll_ctl(m_epfd, EPOLL_CTL_ADD, fd, &ev) == 0) {
            m_conn_cnt++;
        } else {
            delete conn;
        }
    }
}

void Reactor ::handleClose(Connection* conn) {
    epoll_ctl(m_epfd, EPOLL_CTL_DEL, conn->getFd(), nullptr);
    delete conn;
    m_conn_cnt--;
}

void Reactor ::handleRead(Connection* conn) {
    std::string data;
    if (!conn->recvData(data)) {
        handleClose(conn);
        return;
    }
    if (data.empty()) {
        return;
    }
    std::cout << data << std::endl;
}