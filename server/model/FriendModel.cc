#include "FriendModel.h"

#include <cstdio>
#include <string>

FriendModel::FriendModel(MySQLPool* pool) : m_user_model(pool) {
    m_mysql.setPool(pool);
}

bool FriendModel::insert(int user_id, int friend_id) {
    MYSQL* conn;
    if ((conn = m_mysql.transaction()) == nullptr) {
        return false;
    }
    char sql1[1024] = {0};
    snprintf(sql1, sizeof(sql1),
             "INSERT INTO friend(user_id, friend_id, status) "
             "VALUES(%d,%d,%d);",
             user_id, friend_id, 1);
    char sql2[1024] = {0};
    snprintf(sql2, sizeof(sql2),
             "INSERT INTO friend(user_id, friend_id, status) "
             "VALUES(%d,%d,%d);",
             friend_id, user_id, 0);
    if (mysql_query(conn, sql1) == 0 && mysql_query(conn, sql2) == 0 &&
        m_mysql.commit(conn)) {
        return true;
    } else {
        m_mysql.rollback(conn);
        return false;
    }
}

bool FriendModel::deleteFriend(int user_id, int friend_id) {
    char sql[1024] = {0};
    snprintf(sql, sizeof(sql),
             "DELETE FROM friend "
             "WHERE (user_id = %d AND friend_id = %d) "
             "OR (user_id = %d AND friend_id = %d);",
             user_id, friend_id, friend_id, user_id);
    return m_mysql.update(sql);
}

bool FriendModel::updateStatus(int user_id, int friend_id, int status) {
    char sql[1024] = {0};
    snprintf(sql, sizeof(sql),
             "UPDATE friend SET status = %d "
             "WHERE user_id = %d AND friend_id = %d;",
             status, user_id, friend_id);
    return m_mysql.update(sql);
}

bool FriendModel::addFriend(int user_id, int friend_id) {
    MYSQL* conn;
    if ((conn = m_mysql.transaction()) == nullptr) {
        return false;
    }
    char sql1[1024] = {0};
    snprintf(sql1, sizeof(sql1),
             "UPDATE friend SET status = 2 "
             "WHERE user_id = %d AND friend_id = %d;",
             user_id, friend_id);
    char sql2[1024] = {0};
    snprintf(sql2, sizeof(sql2),
             "UPDATE friend SET status = 2 "
             "WHERE user_id = %d AND friend_id = %d;",
             friend_id, user_id);
    if (mysql_query(conn, sql1) == 0 && mysql_query(conn, sql2) == 0 &&
        m_mysql.commit(conn)) {
        return true;
    } else {
        m_mysql.rollback(conn);
        return false;
    }
}

std::vector<User> FriendModel::queryFriends(int user_id) {
    char sql[1024] = {0};
    snprintf(sql, sizeof(sql),
             "SELECT friend_id FROM friend "
             "WHERE user_id = %d AND status > 1;",
             user_id);
    std::vector<User> friends;
    if (!m_mysql.query(sql)) {
        return friends;
    }
    while (m_mysql.next()) {
        int friend_id = std::stoi(m_mysql.value(0));
        User user;
        if (m_user_model.queryById(friend_id, user)) {
            friends.emplace_back(user);
        }
    }
    return friends;
}

int FriendModel::isFriend(int user_id, int friend_id) {
    char sql[1024] = {0};
    snprintf(sql, sizeof(sql),
             "SELECT status FROM friend "
             "WHERE user_id = %d AND friend_id = %d;",
             user_id, friend_id);
    if (m_mysql.query(sql) && m_mysql.next()) {
        return std::stoi(m_mysql.value(0));
    }
    return -1;
}

bool FriendModel::removeAll(int user_id) {
    char sql[1024] = {0};
    snprintf(sql, sizeof(sql),
             "DELETE FROM friend WHERE user_id = %d OR friend_id = %d;",
             user_id, user_id);
    return m_mysql.update(sql);
}