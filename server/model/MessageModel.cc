#include "MessageModel.h"

MessageModel::MessageModel() {
    m_mysql.connect("chatserver", "123456", "chatroom", "127.0.0.1");
}

bool MessageModel::insert(int sender_id,
                          int receiver_id,
                          int type,
                          const std::string& msg) {
    char sql[1024] = {0};
    snprintf(sql, sizeof(sql),
             "INSERT INTO message(sender_id, receiver_id, type, content) "
             "VALUES(%d,%d,%d,'%s');",
             sender_id, receiver_id, type, msg.c_str());
    return m_mysql.update(sql);
}

std::vector<json> MessageModel::queryHistory(int user_id, int friend_id) {
    char sql[1024] = {0};
    snprintf(sql, sizeof(sql),
             "SELECT sender_id, content, send_time FROM ("
             "SELECT sender_id, content, send_time FROM message WHERE type = 0 "
             "AND ((sender_id = %d AND receiver_id = %d) "
             "OR (sender_id = %d AND receiver_id = %d)) "
             "ORDER BY send_time DESC LIMIT 100"
             ") AS t ORDER BY send_time ASC;",
             user_id, friend_id, friend_id, user_id);
    std::vector<json> history;
    if (!m_mysql.query(sql)) {
        return history;
    }
    while (m_mysql.next()) {
        json js;
        js["sender_id"] = std::stoi(m_mysql.value(0));
        js["content"] = m_mysql.value(1);
        js["time"] = m_mysql.value(2);
        history.emplace_back(js);
    }
    return history;
}