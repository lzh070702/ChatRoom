#include <cstdio>
#include <string>
#include "../database/MySQL.h"
#include "UserModel.h"

UserModel::UserModel() {
    m_mysql.connect("chatserver", "123456", "chatroom", "127.0.0.1");
}

bool UserModel::insert(User& user) {
    char sql[1024] = {0};
    snprintf(sql, sizeof(sql),
             "INSERT INTO user(name, email, password, state) "
             "VALUES('%s','%s','%s',%d);",
             user.getName().c_str(), user.getEmail().c_str(),
             user.getPassword().c_str(), user.getState());
    return m_mysql.update(sql);
}

bool UserModel::queryByEmail(const std::string& email, User& user) {
    char sql[1024] = {0};
    snprintf(sql, sizeof(sql),
             "SELECT name, email, password, state FROM user "
             "WHERE email = '%s';",
             email.c_str());
    if (m_mysql.query(sql) && m_mysql.next()) {
        user.setId(std::stoi(m_mysql.value(0)));
        user.setName(m_mysql.value(1));
        user.setEmail(m_mysql.value(2));
        user.setPassword(m_mysql.value(3));
        user.setState(std::stoi(m_mysql.value(4)));
        return true;
    }
    return false;
}

bool UserModel::updateState(int id, int state) {
    char sql[1024] = {0};
    snprintf(sql, sizeof(sql),
             "UPDATE user SET state = %d "
             "WHERE id = %d;",
             state, id);
    return m_mysql.update(sql);
}

bool UserModel::queryById(int id, User& user) {
    char sql[1024] = {0};
    snprintf(sql, sizeof(sql),
             "SELECT id, name, email, password, state FROM user "
             "WHERE id = %d;",
             id);
    if (m_mysql.query(sql) && m_mysql.next()) {
        user.setId(std::stoi(m_mysql.value(0)));
        user.setName(m_mysql.value(1));
        user.setEmail(m_mysql.value(2));
        user.setPassword(m_mysql.value(3));
        user.setState(std::stoi(m_mysql.value(4)));
        return true;
    }
    return false;
}

bool UserModel::resetState() {
    return m_mysql.update("UPDATE user SET state = 0;");
}