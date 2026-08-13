#pragma once

#include <string>

class TcpClient {
   public:
    ~TcpClient();

    bool connectServer(std::string ip, int port);
    std::string recvData();
    bool sendData(std::string msg);
    void closeFd();
    int getFd() const;

   private:
    int m_fd{-1};
    std::string m_buf;
};
