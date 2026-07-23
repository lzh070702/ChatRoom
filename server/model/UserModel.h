#pragma once

#include "../database/MySQL.h"
#include "User.h"

class UserModel {
   public:
    UserModel();
    bool insert(User& user);
    bool queryByEmail(const std::string& email, User& user);
    bool updateState(int id, int state);
    bool queryById(int id, User& user);
    bool resetState();

   private:
    MySQL m_mysql;
};