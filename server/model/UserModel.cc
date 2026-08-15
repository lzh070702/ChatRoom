#include <cstdio>
#include <string>
#include <vector>

#include "UserModel.h"

UserModel::UserModel(MySQLPool* pool) {
    m_mysql.setPool(pool);
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

bool UserModel::queryById(int id, User& user) {
    char sql[1024] = {0};
    snprintf(sql, sizeof(sql),
             "SELECT id, name, email, password, state FROM user "
             "WHERE id = %d;",
             id);
    std::vector<std::vector<std::string>> rows;
    if (!m_mysql.queryAll(sql, rows) || rows.empty()) {
        return false;
    }
    auto& row = rows[0];
    user.setId(std::stoi(row[0]));
    user.setName(row[1]);
    user.setEmail(row[2]);
    user.setPassword(row[3]);
    user.setState(std::stoi(row[4]));
    return true;
}

bool UserModel::queryByEmail(const std::string& email, User& user) {
    char sql[1024] = {0};
    snprintf(sql, sizeof(sql),
             "SELECT id, name, email, password, state FROM user "
             "WHERE email = '%s';",
             email.c_str());
    std::vector<std::vector<std::string>> rows;
    if (!m_mysql.queryAll(sql, rows) || rows.empty()) {
        return false;
    }
    auto& row = rows[0];
    user.setId(std::stoi(row[0]));
    user.setName(row[1]);
    user.setEmail(row[2]);
    user.setPassword(row[3]);
    user.setState(std::stoi(row[4]));
    return true;
}

bool UserModel::updateState(int id, int state) {
    char sql[1024] = {0};
    snprintf(sql, sizeof(sql),
             "UPDATE user SET state = %d "
             "WHERE id = %d;",
             state, id);
    return m_mysql.update(sql);
}

bool UserModel::resetState() {
    return m_mysql.update("UPDATE user SET state = 0;");
}

bool UserModel::updatePassword(int id, const std::string& password) {
    char sql[1024] = {0};
    snprintf(sql, sizeof(sql),
             "UPDATE user SET password = '%s' WHERE id = %d;",
             password.c_str(), id);
    return m_mysql.update(sql);
}

bool UserModel::remove(int id) {
    char sql[1024] = {0};
    snprintf(sql, sizeof(sql), "DELETE FROM user WHERE id = %d;", id);
    return m_mysql.update(sql);
}
