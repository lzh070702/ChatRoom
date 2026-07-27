#pragma once

#include <vector>

#include "../database/MySQL.h"
#include "User.h"
#include "UserModel.h"

class FriendModel {
   public:
    FriendModel();
    bool insert(int user_id, int friend_id);
    std::vector<User> queryFriends(int user_id);
    bool deleteFriend(int userid, int friend_id);
    bool isFriend(int user_id, int friend_id);

   private:
    MySQL m_mysql;
    UserModel m_user_model;
};