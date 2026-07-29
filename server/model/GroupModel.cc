#include "GroupModel.h"

GroupModel::GroupModel() {
    m_mysql.connect("chatserver", "123456", "chatroom", "127.0.0.1");
}

bool GroupModel::createGroup(std::string name,
                             int owner_id,
                             unsigned long long group_id) {
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
                 "VALUES(%llu,%d,4);",
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

std::vector<int> GroupModel::queryGroups(int user_id) {}

std::vector<int> GroupModel::queryMembers(int group_id) {}