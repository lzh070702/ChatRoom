#include "MessageModel.h"

MessageModel::MessageModel(MySQLPool* pool) {
    m_mysql.setPool(pool);
}

int MessageModel::insertFile(const std::string& file) {
    std::string sql =
        "INSERT INTO file(file) "
        "VALUES(" +
        m_mysql.escape(file) + ");";
    return m_mysql.updateAndGetId(sql);
}

bool MessageModel::insertBatch(const std::vector<std::string>& lines) {
    if (lines.empty()) {
        return true;
    }
    MYSQL* conn;
    if ((conn = m_mysql.transaction()) == nullptr) {
        return false;
    }
    for (const auto& line : lines) {
        json msg = json::parse(line);
        int sid = msg["sid"];
        int rid = msg["rid"];
        int type = msg["type"] == 13 ? 0 : 1;
        int is_file = msg["msg_type"];
        std::string sql =
            "INSERT INTO message(sender_id, receiver_id, type, content, "
            "is_file) VALUES(" +
            std::to_string(sid) + "," + std::to_string(rid) + "," +
            std::to_string(type) + ",'" + m_mysql.escape(conn, msg["msg"]) +
            "'," + std::to_string(is_file) + ");";
        if (mysql_query(conn, sql.c_str())) {
            m_mysql.rollback(conn);
            return false;
        }
    }
    return m_mysql.commit(conn);
}

bool MessageModel::updateContent(int id, const std::string& file) {
    std::string sql = "UPDATE file SET file = '" + m_mysql.escape(file) +
                      "' WHERE id = " + std::to_string(id) + ";";
    return m_mysql.update(sql);
}

std::vector<json> MessageModel::queryHistory(int user_id,
                                             int friend_id,
                                             int scope) {
    char sql[1024] = {0};
    const char* limit = (scope == 0) ? "LIMIT 19" : "";
    snprintf(sql, sizeof(sql),
             "SELECT sender_id, content, send_time, is_file FROM ("
             "SELECT sender_id, content, send_time, is_file, id FROM message "
             "WHERE type = 0 "
             "AND ((sender_id = %d AND receiver_id = %d) "
             "OR (sender_id = %d AND receiver_id = %d)) "
             "ORDER BY send_time DESC, id DESC %s"
             ") AS t ORDER BY send_time ASC, id ASC;",
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
             "SELECT m.sender_id, u.name, m.content, m.send_time, m.is_file, "
             "m.id "
             "FROM message m JOIN user u ON m.sender_id = u.id "
             "WHERE m.type = 1 AND m.receiver_id = %d "
             "ORDER BY m.send_time DESC, m.id DESC %s"
             ") AS t ORDER BY t.send_time ASC, t.id ASC;",
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
    snprintf(sql, sizeof(sql), "DELETE FROM message WHERE sender_id = %d;",
             user_id);
    if (!m_mysql.update(sql)) {
        return false;
    }
    snprintf(sql, sizeof(sql),
             "DELETE FROM message WHERE type = 0 AND receiver_id = %d;",
             user_id);
    return m_mysql.update(sql);
}