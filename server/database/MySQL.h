#pragma once
#include <mysql/mysql.h>
#include <chrono>
#include <string>

class MySQL {
   public:
    MySQL();
    ~MySQL();

    bool connect(const std::string& user,
                 const std::string& passwd,
                 const std::string& dbName,
                 const std::string& ip,
                 unsigned short port = 3306);
    bool update(const std::string& sql);
    bool query(const std::string& sql);
    bool next();
    std::string value(int index);
    bool transaction();
    bool commit();
    bool rollback();
    void refreshAliveTime();
    long long getAliveTime();

   private:
    void freeResult();

    MYSQL* m_conn = nullptr;
    MYSQL_RES* m_result = nullptr;
    MYSQL_ROW m_row = nullptr;
    std::chrono::steady_clock::time_point m_aliveTime;
};