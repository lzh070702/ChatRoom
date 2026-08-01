#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "../database/MySQL.h"

using json = nlohmann::json;

class MessageModel {
   public:
    MessageModel();
    bool insert(int sender_id,
                int receiver_id,
                int type,
                const std::string& msg);
    std::vector<json> queryHistory(int user_id, int friend_id);
    std::vector<json> queryGroupHistory(int group_id);

   private:
    MySQL m_mysql;
};