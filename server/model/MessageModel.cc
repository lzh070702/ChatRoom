#include "MessageModel.h"

MessageModel::MessageModel(MySQLPool* pool) {
    m_mysql.setPool(pool);
}

int MessageModel::insert(int sender_id,
                         int receiver_id,
                         int type,
                         const std::string& msg,
                         bool is_file) {
    std::string sql =
        "INSERT INTO message(sender_id, receiver_id, type, content, is_file) "
        "VALUES(" +
        std::to_string(sender_id) + "," + std::to_string(receiver_id) + "," +
        std::to_string(type) + ",'" + m_mysql.escape(msg) + "'," +
        (is_file ? "1" : "0") + ");";
    return m_mysql.updateAndGetId(sql);
}

bool MessageModel::updateContent(int msg_id, const std::string& content) {
    std::string sql = "UPDATE message SET content = '" +
                      m_mysql.escape(content) +
                      "' WHERE id = " + std::to_string(msg_id) + ";";
    return m_mysql.update(sql);
}

std::vector<json> MessageModel::queryHistory(int user_id,
                                             int friend_id,
                                             int scope) {
    char sql[1024] = {0};
    const char* limit = (scope == 0) ? "LIMIT 19" : "";
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
             "SELECT t.sender_id, t.name, t.content, t.send_time, t.is_file "
             "FROM ("
             "SELECT m.sender_id, u.name, m.content, m.send_time, m.is_file "
             "FROM message m JOIN user u ON m.sender_id = u.id "
             "WHERE m.type = 1 AND m.receiver_id = %d "
             "ORDER BY m.send_time DESC %s"
             ") AS t ORDER BY t.send_time ASC;",
             group_id, limit);
    std::vector<json> history;
    std::vector<std::vector<std::string>> rows;
    if (!m_mysql.queryAll(sql, rows)) {
        return history;
    }
    for (auto& row : rows) {
        json js;
        js["sender_id"] = std::stoi(row[0]);
        js["sender_name"] = row[1];
        js["content"] = row[2];
        js["time"] = row[3];
        js["is_file"] = std::stoi(row[4]);
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