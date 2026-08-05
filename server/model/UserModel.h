#pragma once

#include "../database/MySQL.h"
#include "User.h"

class MySQLPool;

class UserModel {
   public:
    UserModel(MySQLPool* pool);
    bool insert(User& user);
    bool queryById(int id, User& user);
    bool queryByEmail(const std::string& email, User& user);
    bool updateState(int id, int state);
    bool resetState();
    bool updatePassword(int id, const std::string& password);
    bool remove(int id);

   private:
    MySQL m_mysql;
};