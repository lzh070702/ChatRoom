#include <fcntl.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <iostream>  ////////////////////////////

#include "Reactor.h"

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
                Connection* raw = static_cast<Connection*>(events[i].data.ptr);
                int fd = raw->getFd();
                auto it = m_conns.find(fd);
                if (it == m_conns.end()) {
                    continue;
                }
                auto conn = it->second;
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
        std::lock_guard<std::mutex> lock(m_mtx);
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
        std::lock_guard<std::mutex> lock(m_mtx);
        cfds.swap(m_cds);
    }
    for (const auto& fd : cfds) {
        fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
        ev.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
        auto conn = std::make_shared<Connection>(fd, this);
        ev.data.ptr = conn.get();
        if (epoll_ctl(m_epfd, EPOLL_CTL_ADD, fd, &ev) == 0) {
            m_conns[fd] = conn;
            m_conn_cnt++;
        }
    }
}

void Reactor ::handleClose(std::shared_ptr<Connection> conn) {
    epoll_ctl(m_epfd, EPOLL_CTL_DEL, conn->getFd(), nullptr);
    ChatService::instance().logout(conn);
    conn->setUserId(-1);
    m_conns.erase(conn->getFd());
    m_conn_cnt--;
}

void Reactor ::handleRead(std::shared_ptr<Connection> conn) {
    if (!conn->recvData()) {
        handleClose(conn);
        return;
    }
    std::string msg;
    while (conn->getMessage(msg)) {
        auto js = json::parse(msg);
        ChatService::instance().handle(conn, js);
    }
}

void Reactor ::handleWrite(std::shared_ptr<Connection> conn,
                           const std::string& data) {
    if (data.empty()) {
        return;
    }
    if (!conn->sendData(data + '\n')) {
        handleClose(conn);
        return;
    }
}
