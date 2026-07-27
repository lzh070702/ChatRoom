#include "Connection.h"
#include <unistd.h>
#include <cerrno>

Connection::Connection(int fd, Reactor* reactor)
    : m_fd(fd), m_reactor(reactor) {}

Connection::~Connection() {
    closeFd();
}

int Connection::getFd() const {
    return m_fd;
}

bool Connection::recvData(std::string& data) {
    if (m_fd == -1) {
        return false;
    }
    data.clear();
    char buf[4096];
    while (true) {
        ssize_t n = read(m_fd, buf, sizeof(buf));
        if (n > 0) {
            data.append(buf, static_cast<size_t>(n));
        } else if (n == 0 ||
                   (n < 0 && (errno != EAGAIN && errno != EWOULDBLOCK))) {
            return false;
        } else if (errno == EINTR) {
            continue;
        } else {
            break;
        }
    }
    return !data.empty();
}

bool Connection::sendData(const std::string& data) {
    if (m_fd == -1) {
        return false;
    }
    size_t totalSent = 0;
    while (totalSent < data.size()) {
        ssize_t n =
            write(m_fd, data.data() + totalSent, data.size() - totalSent);
        if (n > 0) {
            totalSent += static_cast<size_t>(n);
        } else if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            return false;
        } else if (errno == EINTR) {
            continue;
        } else {
            break;
        }
    }
    return totalSent == data.size();
}

void Connection::closeFd() {
    if (m_fd != -1) {
        close(m_fd);
        m_fd = -1;
    }
}

bool Connection::isClosed() const {
    return m_fd == -1;
}

void Connection::setUserId(int id) {
    m_user_id = id;
}

int Connection::getUserId() const {
    return m_user_id;
}

Reactor* Connection::getReactor() const {
    return m_reactor;
}
