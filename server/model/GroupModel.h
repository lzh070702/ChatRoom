#pragma once

#include <string>

#include "../database/MySQL.h"

class GroupModel {
   public:
    GroupModel();
    bool createGroup(std::string name,
                     int owner_id,
                     unsigned long long group_id);
    std::vector<int> queryGroups(int user_id);
    std::vector<int> queryMembers(int group_id);

   private:
    MySQL m_mysql;
};