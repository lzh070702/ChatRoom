#pragma once

#include <mysql/mysql.h>
#include <string>

#include "MySQLPool.h"

class MySQL {
   public:
    ~MySQL();

    void setPool(MySQLPool* pool);
    bool update(const std::string& sql);
    int updateAndGetId(const std::string& sql);
    bool query(const std::string& sql);
    bool next();
    std::string value(int index);
    MYSQL* transaction();
    bool commit(MYSQL* conn);
    bool rollback(MYSQL* conn);

   private:
    void freeResult();

    MySQLPool* m_pool = nullptr;
    MYSQL_RES* m_result = nullptr;
    MYSQL_ROW m_row = nullptr;
};