#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "../database/MySQL.h"

using json = nlohmann::json;

class MySQLPool;

class MessageModel {
   public:
    MessageModel(MySQLPool* pool);
    int insertFile(const std::string& file);
    bool insertBatch(const std::vector<std::string>& lines);
    bool updateContent(int id, const std::string& file);
    std::vector<json> queryHistory(int user_id, int friend_id, int scope = 0);
    std::vector<json> queryGroupHistory(int group_id, int scope = 0);
    bool removeAll(int user_id);

   private:
    MySQL m_mysql;
};