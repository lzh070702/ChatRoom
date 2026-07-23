#include "MySQL.h"

MySQL::MySQL() {
    m_conn = mysql_init(nullptr);
    mysql_set_character_set(m_conn, "utf8");
}

MySQL::~MySQL() {
    if (m_conn != nullptr) {
        mysql_close(m_conn);
    }
    freeResult();
}

bool MySQL::connect(const std::string& user,
                    const std::string& passwd,
                    const std::string& dbName,
                    const std::string& ip,
                    unsigned short port) {
    MYSQL* ptr =
        mysql_real_connect(m_conn, ip.c_str(), user.c_str(), passwd.c_str(),
                           dbName.c_str(), port, nullptr, 0);
    return ptr != nullptr;
}

bool MySQL::update(const std::string& sql) {
    if (mysql_query(m_conn, sql.c_str())) {
        return false;
    }
    return true;
}

bool MySQL::query(const std::string& sql) {
    freeResult();
    if (mysql_query(m_conn, sql.c_str())) {
        return false;
    }
    m_result = mysql_store_result(m_conn);
    return true;
}

bool MySQL::next() {
    if (m_result != nullptr) {
        m_row = mysql_fetch_row(m_result);
        if (m_row != nullptr) {
            return true;
        }
    }
    return false;
}

std::string MySQL::value(int index) {
    int fieldCount = mysql_num_fields(m_result);
    if (index >= fieldCount || index < 0) {
        return std::string();
    }
    char* val = m_row[index];
    unsigned long length = mysql_fetch_lengths(m_result)[index];
    return std::string(val, length);
}

bool MySQL::transaction() {
    return mysql_autocommit(m_conn, false);
}

bool MySQL::commit() {
    return mysql_commit(m_conn);
}

bool MySQL::rollback() {
    return mysql_rollback(m_conn);
}

void MySQL::refreshAliveTime() {
    m_aliveTime = std::chrono::steady_clock::now();
}

long long MySQL::getAliveTime() {
    std::chrono::nanoseconds res =
        std::chrono::steady_clock::now() - m_aliveTime;
    std::chrono::milliseconds millsec =
        std::chrono::duration_cast<std::chrono::milliseconds>(res);
    return millsec.count();
}

void MySQL::freeResult() {
    if (m_result != nullptr) {
        mysql_free_result(m_result);
        m_result = nullptr;
        m_row = nullptr;
    }
}