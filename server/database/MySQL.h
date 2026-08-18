#pragma once

#include <mysql/mysql.h>
#include <string>
#include <vector>

#include "MySQLPool.h"

class MySQL {
   public:
    void setPool(MySQLPool* pool);
    bool update(const std::string& sql);
    int updateAndGetId(const std::string& sql);
    bool queryAll(const std::string& sql,
                  std::vector<std::vector<std::string>>& rows);
    std::string escape(const std::string& str);
    std::string escape(MYSQL* conn, const std::string& str);
    MYSQL* transaction();
    bool commit(MYSQL* conn);
    bool rollback(MYSQL* conn);

   private:
    MySQLPool* m_pool = nullptr;
};