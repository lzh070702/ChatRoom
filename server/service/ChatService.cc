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
        conn->getReactor()->handleWrite(
            conn, R"({"type":0,"code":0,"msg":"异常请求"})");
        return;
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
    m_handlers[3] = [this](Connection* c, const json& j) { sendRequest(c, j); };
    m_handlers[4] = [this](Connection* c, const json& j) {
        processRequest(c, j);
    };
    m_handlers[5] = [this](Connection* c, const json& j) {
        queryFriends(c, j);
    };
    m_handlers[6] = [this](Connection* c, const json& j) {
        deleteFriend(c, j);
    };
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

void ChatService::sendRequest(Connection* conn, const json& js) {
    int user_id = conn->getUserId();
    std::string email = js["email"];
    User user;
    if (!m_user_model.queryByEmail(email, user)) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":3,"code":0,"msg":"该用户不存在"})");
        return;
    }
    int friend_id = user.getId();
    if (friend_id == user_id) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":3,"code":0,"msg":"不能添加自己为好友"})");
        return;
    }
    if (m_friend_model.isFriend(user_id, friend_id) != -1) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":3,"code":0,"msg":"请勿重复添加"})");
        return;
    }
    if (!m_friend_model.insert(user_id, friend_id)) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":3,"code":0,"msg":"好友申请发送失败"})");
        return;
    }
    conn->getReactor()->handleWrite(
        conn, R"({"type":3,"code":1,"msg":"成功发送申请"})");
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_user_conn.find(friend_id);
    if (it != m_user_conn.end()) {
        it->second->getReactor()->handleWrite(
            it->second, R"({"type":3,"code":2,"msg":"好友申请","user_id":)" +
                            std::to_string(user_id) + R"(,"name":")" +
                            user.getName() + R"(","email":")" + email +
                            R"("})");
    }
}

void ChatService::processRequest(Connection* conn, const json& js) {
    int user_id = conn->getUserId();
    int decision = js["decision"];
    int friend_id = js["id"];
    User user;
    if (!m_user_model.queryById(friend_id, user)) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":4,"code":0,"msg":"该用户已注销"})");
        return;
    }
    if (decision == 1) {
        m_friend_model.addFriend(user_id, friend_id);
        conn->getReactor()->handleWrite(
            conn, R"({"type":4,"code":1,"msg":"成功添加为好友"})");
        return;
    }
    m_friend_model.deleteFriend(user_id, friend_id);
    conn->getReactor()->handleWrite(
        conn, R"({"type":4,"code":1,"msg":"已拒绝该申请"})");
}

void ChatService::queryFriends(Connection* conn, const json& js) {
    int user_id = conn->getUserId();
    std::vector<User> friends = m_friend_model.queryFriends(conn->getUserId());
    json response;
    response["type"] = 5;
    response["code"] = 1;
    for (auto& user : friends) {
        json fri;
        fri["name"] = user.getName();
        fri["email"] = user.getEmail();
        fri["state"] = user.getState();
        fri["status"] = m_friend_model.isFriend(user_id, user.getId());
        response["friends"].push_back(fri);
    }
    conn->getReactor()->handleWrite(conn, response.dump());
}

void ChatService::deleteFriend(Connection* conn, const json& js) {
    int user_id = conn->getUserId();
    int friend_id = js["id"];
    if (!m_friend_model.deleteFriend(user_id, friend_id)) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":6,"code":1,"msg":"删除好友失败"})");
    }
    conn->getReactor()->handleWrite(
        conn, R"({"type":6,"code":1,"msg":"成功删除该好友"})");
}