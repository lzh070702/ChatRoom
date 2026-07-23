#pragma once
#include <cstdint>
#include <string>

class Connection {
   public:
    explicit Connection(int fd);
    ~Connection();
    int getFd() const;
    bool recvData(std::string& data);
    bool sendData(const std::string& data);
    void closeFd();
    bool isClosed() const;

   private:
    int m_fd{-1};
};