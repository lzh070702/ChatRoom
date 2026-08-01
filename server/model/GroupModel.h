#pragma once

#include <nlohmann/json.hpp>
#include <string>

#include "../database/MySQL.h"

using json = nlohmann::json;

class GroupModel {
   public:
    GroupModel();
    bool createGroup(std::string name, int owner_id, int& group_id);
    std::vector<json> queryGroups(int user_id);
    std::vector<int> queryMembers(int group_id);
    bool groupExist(int group_id);
    bool isInGroup(int group_id, int user_id);
    bool applyGroup(int group_id, int user_id, std::vector<int>& users);
    bool processGroupRequest(int group_id, int user_id, bool agree);
    int getRole(int group_id, int user_id);
    bool delMember(int group_id, int user_id);
    bool dissolveGroup(int group_id);
    bool promoteAdmin(int group_id, int target_id);
    bool demoteAdmin(int group_id, int target_id);

   private:
    MySQL m_mysql;
};