#include "FriendModel.h"

#include <cstdio>
#include <string>

FriendModel::FriendModel() {
    m_mysql.connect("chatserver", "123456", "chatroom", "127.0.0.1");
}

bool FriendModel::insert(int user_id, int friend_id) {
    char sql[1024] = {0};
    snprintf(sql, sizeof(sql),
             "INSERT INTO friend(user_id, friend_id, status, remark) "
             "VALUES(%d,%d,%d,'%s');",
             user_id, friend_id, 0, "");
    return m_mysql.update(sql);
}

std::vector<User> FriendModel::queryFriends(int user_id) {
    char sql[1024] = {0};
    snprintf(sql, sizeof(sql),
             "SELECT friend_id FROM friend "
             "WHERE user_id = %d AND status = 1;",
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

bool FriendModel::deleteFriend(int user_id, int friend_id) {
    char sql[1024] = {0};
    snprintf(sql, sizeof(sql),
             "DELETE FROM friend "
             "WHERE (user_id = %d AND friend_id = %d) "
             "OR (user_id = %d AND friend_id = %d);",
             user_id, friend_id, friend_id, user_id);
    return m_mysql.update(sql);
}

bool FriendModel::isFriend(int user_id, int friend_id) {
    char sql[1024] = {0};
    snprintf(sql, sizeof(sql),
             "SELECT status FROM friend "
             "WHERE user_id = %d AND friend_id = %d;",
             user_id, friend_id);
    if (m_mysql.query(sql) && m_mysql.next() &&
        (std::stoi(m_mysql.value(0)) == 1)) {
        return true;
    }
    return false;
}