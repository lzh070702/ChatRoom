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
    int insert(int sender_id,
               int receiver_id,
               int type,
               const std::string& msg,
               bool is_file);
    std::vector<json> queryHistory(int user_id, int friend_id);
    std::vector<json> queryGroupHistory(int group_id);
    bool removeAll(int user_id);

   private:
    MySQL m_mysql;
};