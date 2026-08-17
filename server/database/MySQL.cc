#include <glog/logging.h>

#include "MySQL.h"

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

bool MySQL::queryAll(const std::string& sql,
                     std::vector<std::vector<std::string>>& rows) {
    rows.clear();
    MYSQL* conn = m_pool->borrow();
    if (mysql_query(conn, sql.c_str())) {
        LOG(ERROR) << "MySQL query failed: " << mysql_error(conn);
        m_pool->returnConn(conn);
        return false;
    }
    MYSQL_RES* result = mysql_store_result(conn);
    m_pool->returnConn(conn);
    if (result == nullptr) {
        return true;
    }
    int field_count = mysql_num_fields(result);
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result)) != nullptr) {
        unsigned long* lengths = mysql_fetch_lengths(result);
        std::vector<std::string> cols;
        cols.reserve(field_count);
        for (int i = 0; i < field_count; ++i) {
            if (row[i] == nullptr) {
                cols.emplace_back();
            } else {
                cols.emplace_back(row[i], static_cast<size_t>(lengths[i]));
            }
        }
        rows.emplace_back(std::move(cols));
    }
    mysql_free_result(result);
    return true;
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
    mysql_autocommit(conn, true);
    m_pool->returnConn(conn);
    return res == 0;
}

bool MySQL::rollback(MYSQL* conn) {
    bool res = mysql_rollback(conn);
    mysql_autocommit(conn, true);
    m_pool->returnConn(conn);
    return res == 0;
}