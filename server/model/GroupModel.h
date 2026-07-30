#pragma once

#include <nlohmann/json.hpp>
#include <string>

#include "../database/MySQL.h"

using json = nlohmann::json;

class GroupModel {
   public:
    GroupModel();
    bool createGroup(std::string name, int owner_id, int group_id);
    std::vector<json> queryGroups(int user_id);
    std::vector<int> queryMembers(int group_id);
    bool groupExist(int group_id);
    bool isInGroup(int group_id, int user_id);
    bool applyGroup(int group_id, int user_id,std::vector<int>& users);
    bool processGroupRequest(int group_id, int user_id, bool agree);

   private:
    MySQL m_mysql;
};