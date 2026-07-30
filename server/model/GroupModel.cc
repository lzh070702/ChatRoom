#include "GroupModel.h"

GroupModel::GroupModel() {
    m_mysql.connect("chatserver", "123456", "chatroom", "127.0.0.1");
}

bool GroupModel::createGroup(std::string name, int owner_id, int group_id) {
    char sql[1024] = {0};
    if (!m_mysql.transaction()) {
        return false;
    }
    bool success = true;
    snprintf(sql, sizeof(sql),
             "INSERT INTO group_info(name, owner_id) "
             "VALUES('%s',%d);",
             name.c_str(), owner_id);

    if (m_mysql.update(sql)) {
        group_id = m_mysql.getInsertId();
        snprintf(sql, sizeof(sql),
                 "INSERT INTO group_user(group_id,user_id,role)"
                 "VALUES(%d, %d, 3);",
                 group_id, owner_id);
        if (!m_mysql.update(sql)) {
            success = false;
        }
    } else {
        success = false;
    }
    if (success && m_mysql.commit()) {
        return true;
    } else {
        m_mysql.rollback();
        return false;
    }
}

std::vector<json> GroupModel::queryGroups(int user_id) {
    char sql[1024] = {0};
    snprintf(sql, sizeof(sql),
             "SELECT g.id, g.name "
             "FROM group_info g "
             "INNER JOIN group_user gu "
             "ON g.id = gu.group_id "
             "WHERE gu.user_id = %d "
             "AND gu.role > 0;",
             user_id);
    std::vector<json> groups;
    if (!m_mysql.query(sql)) {
        return groups;
    }
    while (m_mysql.next()) {
        json group;
        group["id"] = m_mysql.value(0);
        group["name"] = m_mysql.value(1);
        groups.emplace_back(group);
    }
    return groups;
}

std::vector<int> GroupModel::queryMembers(int group_id) {}

bool GroupModel::groupExist(int group_id) {
    char sql[1024] = {0};
    snprintf(sql, sizeof(sql),
             "SELECT 1 FROM group_info "
             "WHERE id = %d;",
             group_id);
    return m_mysql.query(sql) && m_mysql.next();
}

bool GroupModel::isInGroup(int group_id, int user_id) {
    char sql[1024] = {0};
    snprintf(sql, sizeof(sql),
             "SELECT user_id FROM group_user "
             "WHERE group_id = %d AND user_id = %d",
             group_id, user_id);
    if (!m_mysql.query(sql)) {
        return false;
    }
    if (m_mysql.next()) {
        return true;
    }
    return false;
}

bool GroupModel::applyGroup(int group_id,
                            int user_id,
                            std::vector<int>& users) {
    char sql[1024] = {0};
    snprintf(sql, sizeof(sql),
             "INSERT INTO group_user(group_id, user_id, role) "
             "VALUES(%d, %d, 0);",
             group_id, user_id);
    if (!m_mysql.update(sql)) {
        return false;
    }
    snprintf(sql, sizeof(sql),
             "SELECT user_id FROM group_user "
             "WHERE group_id = %d AND role > 1",
             group_id);
    if (!m_mysql.query(sql)) {
        return false;
    }
    while (m_mysql.next()) {
        users.emplace_back(std::stoi(m_mysql.value(0)));
    }
    return true;
}

bool GroupModel::processGroupRequest(int group_id, int user_id, bool agree) {
    char sql[1024] = {0};
    if (agree) {
        snprintf(sql, sizeof(sql),
                 "UPDATE group_user SET role = 1 "
                 "WHERE group_id = %d AND user_id = %d;",
                 group_id, user_id);
    } else {
        snprintf(sql, sizeof(sql),
                 "DELETE FROM group_user "
                 "WHERE group_id = %d AND user_id = %d;",
                 group_id, user_id);
    }
    return m_mysql.update(sql);
}

