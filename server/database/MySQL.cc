#include <glog/logging.h>

#include "MySQL.h"

MySQL::~MySQL() {
    freeResult();
}

void MySQL::setPool(MySQLPool* pool) {
    m_pool = pool;
}

bool MySQL::update(const std::string& sql) {
    MYSQL* conn = m_pool->borrow();
    if (mysql_query(conn, sql.c_str())) {
        LOG(ERROR) << "MySQL update failed: " << mysql_error(conn);
        m_pool->returnConn(conn);
        return false;
    }
    m_pool->returnConn(conn);
    return true;
}

int MySQL::updateAndGetId(const std::string& sql) {
    MYSQL* conn = m_pool->borrow();
    if (mysql_query(conn, sql.c_str())) {
        LOG(ERROR) << "MySQL update failed: " << mysql_error(conn);
        m_pool->returnConn(conn);
        return -1;
    }
    int id = static_cast<int>(mysql_insert_id(conn));
    m_pool->returnConn(conn);
    return id;
}

bool MySQL::query(const std::string& sql) {
    freeResult();
    MYSQL* conn = m_pool->borrow();
    if (mysql_query(conn, sql.c_str())) {
        LOG(ERROR) << "MySQL query failed: " << mysql_error(conn);
        m_pool->returnConn(conn);
        return false;
    }
    m_result = mysql_store_result(conn);
    m_pool->returnConn(conn);
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

MYSQL* MySQL::transaction() {
    MYSQL* conn = m_pool->borrow();
    if (mysql_autocommit(conn, false) != 0) {
        m_pool->returnConn(conn);
        return nullptr;
    }
    return conn;
}

bool MySQL::commit(MYSQL* conn) {
    bool res = mysql_commit(conn);
    m_pool->returnConn(conn);
    return res == 0;
}

bool MySQL::rollback(MYSQL* conn) {
    bool res = mysql_rollback(conn);
    m_pool->returnConn(conn);
    return res == 0;
}

void MySQL::freeResult() {
    if (m_result != nullptr) {
        mysql_free_result(m_result);
        m_result = nullptr;
        m_row = nullptr;
    }
}