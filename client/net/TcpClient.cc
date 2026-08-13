#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>

#include "TcpClient.h"

TcpClient::~TcpClient() {
    closeFd();
}

bool TcpClient::connectServer(std::string ip, int port) {
    m_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_fd < 0) {
        return false;
    }
    sockaddr_in serv{};
    serv.sin_family = AF_INET;
    serv.sin_port = htons(port);
    if (inet_pton(AF_INET, ip.c_str(), &serv.sin_addr) <= 0) {
        closeFd();
        return false;
    }
    if (connect(m_fd, reinterpret_cast<sockaddr*>(&serv), sizeof(serv)) < 0) {
        closeFd();
        return false;
    }
    int flags = fcntl(m_fd, F_GETFL, 0);
    fcntl(m_fd, F_SETFL, flags | O_NONBLOCK);
    return true;
}

std::string TcpClient::recvData() {
    if (m_fd == -1) {
        return "";
    }
    char buf[4096];
    while (true) {
        ssize_t n = read(m_fd, buf, sizeof(buf));
        if (n > 0) {
            m_buf.append(buf, static_cast<size_t>(n));
        } else if (n == 0) {
            return "";
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            } else if (errno == EINTR) {
                continue;
            } else {
                return "";
            }
        }
    }
    size_t pos = m_buf.find('\n');
    if (pos == std::string::npos) {
        return "";
    }
    std::string msg = m_buf.substr(0, pos);
    m_buf.erase(0, pos + 1);
    return msg;
}

bool TcpClient::sendData(std::string msg) {
    if (m_fd == -1) {
        return false;
    }
    msg += '\n';
    size_t sent = 0;
    while (sent < msg.size()) {
        ssize_t n = write(m_fd, msg.data() + sent, msg.size() - sent);
        if (n > 0) {
            sent += static_cast<size_t>(n);
        } else if (n == 0) {
            return false;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            } else if (errno == EINTR) {
                continue;
            } else {
                return false;
            }
        }
    }
    return sent == msg.size();
}

void TcpClient::closeFd() {
    if (m_fd >= 0) {
        close(m_fd);
        m_fd = -1;
    }
}

int TcpClient::getFd() const {
    return m_fd;
}