#pragma once

#include <sys/epoll.h>
#include <atomic>
#include <mutex>
#include <vector>

#include "../net/Connection.h"
#include "../service/ChatService.h"

class Connection;

class Reactor {
   public:
    Reactor();
    ~Reactor();
    void loop();
    void pushFd(int fd);
    int getWakeupFd() const;
    int getConnCnt() const;
    void handleWrite(Connection* conn, const std::string& data);

   private:
    void handleWakeup();
    void handleClose(Connection* conn);
    void handleRead(Connection* conn);

   private:
    int m_epfd;
    int m_wakeup_fd;
    std::atomic<int> m_conn_cnt{0};
    std::mutex m_mtx;
    std::vector<int> m_cds;

    static constexpr int MAX_EVENTS = 1024;
};