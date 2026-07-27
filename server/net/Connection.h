#pragma once

#include <cstdint>
#include <string>

#include "../reactor/Reactor.h"

class Connection {
   public:
    explicit Connection(int fd, Reactor* reactor);
    ~Connection();
    int getFd() const;
    bool recvData(std::string& data);
    bool sendData(const std::string& data);
    void closeFd();
    bool isClosed() const;
    void setUserId(int id);
    int getUserId() const;
    Reactor* getReactor() const;

   private:
    int m_fd{-1};
    int m_user_id{-1};
    Reactor* m_reactor;
};