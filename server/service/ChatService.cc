#include <iostream>

#include "ChatService.h"

ChatService& ChatService::instance() {
    static ChatService service;
    return service;
}

void ChatService::handle(Connection* conn, const json& js) {
    int type = js["type"];
    auto it = m_handlers.find(type);
    if (it != m_handlers.end()) {
        it->second(conn, js);
    } else {
        // 错误处理
    }
}

/////////////////登出
void ChatService::logout(Connection* conn) {
    int id = conn->getUserId();
    if (id == -1) {
        return;
    }
    m_user_model.updateState(id, 0);
    std::lock_guard<std::mutex> lock(m_mutex);
    m_user_conn.erase(id);
}

ChatService::ChatService() {
    m_handlers[1] = [this](Connection* c, const json& j) { signIn(c, j); };
    m_handlers[2] = [this](Connection* c, const json& j) { signUp(c, j); };
    m_handlers[3] = [this](Connection* c, const json& j) { addFriend(c, j); };
}

void ChatService::signIn(Connection* conn, const json& js) {
    std::string email = js["email"];
    User user;
    if (!m_user_model.queryByEmail(email, user)) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":1,"code":0,"msg":"该邮箱不存在"})");
        return;
    }
    if (user.getPassword() != js["password"]) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":1,"code":0,"msg":"密码错误"})");
        return;
    }
    if (user.getState() == 1) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":1,"code":0,"msg":"该账号已在其他设备登录"})");
        return;
    }
    if (!m_user_model.updateState(user.getId(), 1)) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":1,"code":0,"msg":"登录失败"})");
        return;
    }
    conn->setUserId(user.getId());
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_user_conn[user.getId()] = conn;
    }
    conn->getReactor()->handleWrite(conn,
                                    R"({"type":1,"code":1,"msg":"登录成功"})");
}

void ChatService::signUp(Connection* conn, const json& js) {
    std::string email = js["email"];
    User user;
    if (m_user_model.queryByEmail(email, user)) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":2,"code":0,"msg":"该邮箱已存在"})");
        return;
    }
    user.setName(js["name"]);
    user.setEmail(email);
    user.setPassword(js["password"]);
    user.setState(0);
    if (m_user_model.insert(user)) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":2,"code":1,"msg":"用户信息创建成功"})");
    } else {
        conn->getReactor()->handleWrite(
            conn, R"({"type":2,"code":0,"msg":"用户信息创建失败"})");
    }
}

void ChatService::addFriend(Connection* conn, const json& js) {
    int user_id = js["user_id"];
    int friend_id = js["friend_id"];
    if (user_id == friend_id) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":3,"code":0,"msg":"不能添加自己为好友"})");
        return;
    }
    if (m_friend_odel.isFriend(user_id, friend_id)) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":3,"code":0,"msg":"该好友已添加"})");
        return;
    }
}