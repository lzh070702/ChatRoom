#include "MessageModel.h"

MessageModel::MessageModel(MySQLPool* pool) {
    m_mysql.setPool(pool);
}

int MessageModel::insert(int sender_id,
                         int receiver_id,
                         int type,
                         const std::string& msg,
                         bool is_file) {
    char sql[1024] = {0};
    snprintf(sql, sizeof(sql),
             "INSERT INTO message(sender_id, receiver_id, type, content, "
             "is_file) "
             "VALUES(%d,%d,%d,'%s',%d);",
             sender_id, receiver_id, type, msg.c_str(), is_file ? 1 : 0);
    return m_mysql.updateAndGetId(sql);
}

std::vector<json> MessageModel::queryHistory(int user_id, int friend_id, int scope) {
    char sql[1024] = {0};
    const char* limit = (scope == 0) ? "LIMIT 20" : "";
    snprintf(sql, sizeof(sql),
             "SELECT sender_id, content, send_time, is_file FROM ("
             "SELECT sender_id, content, send_time, is_file FROM message "
             "WHERE type = 0 "
             "AND ((sender_id = %d AND receiver_id = %d) "
             "OR (sender_id = %d AND receiver_id = %d)) "
             "ORDER BY send_time DESC %s"
             ") AS t ORDER BY send_time ASC;",
             user_id, friend_id, friend_id, user_id, limit);
    std::vector<json> history;
    std::vector<std::vector<std::string>> rows;
    if (!m_mysql.queryAll(sql, rows)) {
        return history;
    }
    for (auto& row : rows) {
        json js;
        js["sender_id"] = std::stoi(row[0]);
        js["content"] = row[1];
        js["time"] = row[2];
        js["is_file"] = std::stoi(row[3]);
        history.emplace_back(js);
    }
    return history;
}

std::vector<json> MessageModel::queryGroupHistory(int group_id, int scope) {
    char sql[1024] = {0};
    const char* limit = (scope == 0) ? "LIMIT 20" : "";
    snprintf(sql, sizeof(sql),
             "SELECT sender_id, content, send_time, is_file FROM ("
             "SELECT sender_id, content, send_time, is_file FROM message "
             "WHERE type = 1 AND receiver_id = %d "
             "ORDER BY send_time DESC %s"
             ") AS t ORDER BY send_time ASC;",
             group_id, limit);
    std::vector<json> history;
    std::vector<std::vector<std::string>> rows;
    if (!m_mysql.queryAll(sql, rows)) {
        return history;
    }
    for (auto& row : rows) {
        json js;
        js["sender_id"] = std::stoi(row[0]);
        js["content"] = row[1];
        js["time"] = row[2];
        js["is_file"] = std::stoi(row[3]);
        history.emplace_back(js);
    }
    return history;
}

bool MessageModel::removeAll(int user_id) {
    char sql[1024] = {0};
    // 删除该用户发送的消息（私聊 + 群聊）
    snprintf(sql, sizeof(sql), "DELETE FROM message WHERE sender_id = %d;",
             user_id);
    if (!m_mysql.update(sql)) {
        return false;
    }
    // 删除该用户收到的私聊消息
    snprintf(sql, sizeof(sql),
             "DELETE FROM message WHERE type = 0 AND receiver_id = %d;",
             user_id);
    return m_mysql.update(sql);
}