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
    m_handlers[7] = [this](Connection* c, const json& j) { oneChat(c, j); };
    m_handlers[8] = [this](Connection* c, const json& j) {
        queryHistory(c, j);
    };
    m_handlers[9] = [this](Connection* c, const json& j) { createGroup(c, j); };
    m_handlers[10] = [this](Connection* c, const json& j) {
        queryGroups(c, j);
    };
    m_handlers[11] = [this](Connection* c, const json& j) {
        applyGroups(c, j);
    };
    m_handlers[12] = [this](Connection* c, const json& j) {
        processGroupRequest(c, j);
    };
    m_handlers[13] = [this](Connection* c, const json& j) {
        queryMembers(c, j);
    };
    m_handlers[14] = [this](Connection* c, const json& j) { quitGroup(c, j); };
    m_handlers[15] = [this](Connection* c, const json& j) {
        dissolveGroup(c, j);
    };
    m_handlers[16] = [this](Connection* c, const json& j) {
        promoteAdmin(c, j);
    };
    m_handlers[17] = [this](Connection* c, const json& j) {
        demoteAdmin(c, j);
    };
    m_handlers[18] = [this](Connection* c, const json& j) { kickMember(c, j); };
    m_handlers[19] = [this](Connection* c, const json& j) { groupChat(c, j); };
    m_handlers[20] = [this](Connection* c, const json& j) {
        queryGroupHistory(c, j);
    };
}

void ChatService::signIn(Connection* conn, const json& js) {
    std::string email = js["email"];
    User user;
    if (!m_user_model.queryByEmail(email, user)) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":1,"code":0,"msg":"该账号不存在"})");
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
    json response;
    response["type"] = 1;
    response["code"] = 1;
    response["msg"] = "登录成功";
    response["id"] = user.getId();
    response["name"] = user.getName();
    response["email"] = user.getEmail();
    conn->getReactor()->handleWrite(conn, response.dump());
    std::string data;
    std::string request_key = "offline_request:" + std::to_string(user.getId());
    while (m_redis.rpop(request_key, data)) {
        conn->getReactor()->handleWrite(conn, data);
    }
    std::string group_request_key =
        "offline_group_request:" + std::to_string(user.getId());
    while (m_redis.rpop(group_request_key, data)) {
        conn->getReactor()->handleWrite(conn, data);
    }
    std::string msg_key = "offline_msg:" + std::to_string(user.getId());
    while (m_redis.rpop(msg_key, data)) {
        conn->getReactor()->handleWrite(conn, data);
    }
    std::string group_msg_key =
        "offline_group_msg:" + std::to_string(user.getId());
    while (m_redis.rpop(group_msg_key, data)) {
        conn->getReactor()->handleWrite(conn, data);
    }
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
    std::string my_email = js["my_email"];
    std::string name = js["name"];
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

    Connection* friend_conn = nullptr;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_user_conn.find(friend_id);
        if (it != m_user_conn.end()) {
            friend_conn = it->second;
        }
    }
    json response;
    response["type"] = 3;
    response["code"] = 2;
    response["msg"] = "有新的好友申请";
    response["id"] = user_id;
    response["name"] = name;
    response["email"] = my_email;
    if (friend_conn) {
        friend_conn->getReactor()->handleWrite(friend_conn, response.dump());
        conn->getReactor()->handleWrite(
            conn, R"({"type":3,"code":1,"msg":"成功发送申请"})");
        return;
    }
    if (!m_redis.lpush("offline_request:" + std::to_string(friend_id),
                       response.dump())) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":3,"code":0,"msg":"好友申请发送失败"})");
        return;
    }
    conn->getReactor()->handleWrite(
        conn, R"({"type":3,"code":1,"msg":"成功发送申请"})");
}

void ChatService::processRequest(Connection* conn, const json& js) {
    int user_id = conn->getUserId();
    int agree = js["agree"];
    int friend_id = js["id"];
    User user;
    if (!m_user_model.queryById(friend_id, user)) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":4,"code":0,"msg":"该用户已注销"})");
        return;
    }
    if (agree == 1) {
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
    response["msg"] = "成功获取到好友列表";
    for (auto& user : friends) {
        json friend_info;
        friend_info["id"] = user.getId();
        friend_info["name"] = user.getName();
        friend_info["email"] = user.getEmail();
        friend_info["state"] = user.getState();
        friend_info["status"] = m_friend_model.isFriend(user_id, user.getId());
        response["friends"].push_back(friend_info);
    }
    conn->getReactor()->handleWrite(conn, response.dump());
}

void ChatService::deleteFriend(Connection* conn, const json& js) {
    int user_id = conn->getUserId();
    int friend_id = js["id"];
    if (!m_friend_model.deleteFriend(user_id, friend_id)) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":6,"code":0,"msg":"删除好友失败"})");
        return;
    }
    conn->getReactor()->handleWrite(
        conn, R"({"type":6,"code":1,"msg":"成功删除该好友"})");
}

void ChatService::oneChat(Connection* conn, const json& js) {
    int friend_id = js["id"];
    int user_id = conn->getUserId();
    std::string msg = js["msg"];
    User user;
    if (!m_user_model.queryById(friend_id, user)) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":7,"code":0,"msg":"该用户已注销"})");
        return;
    }
    if (m_friend_model.isFriend(user_id, friend_id) != 2) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":7,"code":0,"msg":"不是好友，无法聊天"})");
        return;
    }
    if (!m_message_model.insert(user_id, friend_id, 0, msg)) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":7,"code":0,"msg":"消息发送失败"})");
        return;
    }
    Connection* friend_conn = nullptr;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_user_conn.find(friend_id);
        if (it != m_user_conn.end()) {
            friend_conn = it->second;
        }
    }
    json response;
    response["type"] = 7;
    response["code"] = 1;
    response["id"] = user_id;
    response["msg"] = msg;
    if (friend_conn) {
        friend_conn->getReactor()->handleWrite(friend_conn, response.dump());
        return;
    }
    if (!m_redis.lpush("offline_msg:" + std::to_string(friend_id),
                       response.dump())) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":7,"code":0,"msg":"消息发送失败"})");
    }
}

void ChatService::queryHistory(Connection* conn, const json& js) {
    int friend_id = js["id"];
    int user_id = conn->getUserId();
    std::vector<json> history =
        m_message_model.queryHistory(user_id, friend_id);
    json response;
    response["type"] = 8;
    response["code"] = 1;
    response["msg"] = history;
    conn->getReactor()->handleWrite(conn, response.dump());
}

void ChatService::createGroup(Connection* conn, const json& js) {
    std::string name = js["name"];
    int user_id = conn->getUserId();
    int group_id = 0;
    if (!m_group_model.createGroup(name, user_id, group_id)) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":9,"code":0,"msg":"创建群聊失败"})");
        return;
    }
    json response;
    response["type"] = 9;
    response["code"] = 1;
    response["msg"] = "成功创建群聊";
    response["group_id"] = group_id;
    conn->getReactor()->handleWrite(conn, response.dump());
}

void ChatService::queryGroups(Connection* conn, const json& js) {
    std::vector<json> groups = m_group_model.queryGroups(conn->getUserId());
    json response;
    response["type"] = 10;
    response["code"] = 1;
    response["msg"] = groups;
    conn->getReactor()->handleWrite(conn, response.dump());
}

void ChatService::applyGroups(Connection* conn, const json& js) {
    int group_id = js["group_id"];
    std::string email = js["email"];
    std::string name = js["name"];
    int user_id = conn->getUserId();
    if (!m_group_model.groupExist(group_id)) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":11,"code":0,"msg":"该群聊不存在"})");
        return;
    }
    if (m_group_model.isInGroup(group_id, user_id)) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":11,"code":0,"msg":"禁止重复加入该群"})");
        return;
    }
    std::vector<int> users;
    if (!m_group_model.applyGroup(group_id, user_id, users)) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":11,"code":0,"msg":"入群申请发送失败"})");
        return;
    }
    json response;
    response["type"] = 11;
    response["code"] = 1;
    response["msg"] = "入群申请";
    response["id"] = user_id;
    response["name"] = name;
    response["email"] = email;
    for (int& user : users) {
        Connection* user_conn = nullptr;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto it = m_user_conn.find(user);
            if (it != m_user_conn.end()) {
                user_conn = it->second;
            }
        }
        if (user_conn) {
            user_conn->getReactor()->handleWrite(user_conn, response.dump());
        } else {
            if (!m_redis.lpush("offline_group_request:" + std::to_string(user),
                               response.dump())) {
                conn->getReactor()->handleWrite(
                    conn, R"({"type":11,"code":0,"msg":"入群申请发送失败"})");
                return;
            }
        }
    }
    conn->getReactor()->handleWrite(
        conn, R"({"type":11,"code":1,"msg":"入群申请发送成功"})");
}

void ChatService::processGroupRequest(Connection* conn, const json& js) {
    int group_id = js["group_id"];
    bool agree = js["agree"];
    int applicant_id = js["id"];
    int user_id = conn->getUserId();

    // 1. 检查操作人权限
    int user_role = m_group_model.getRole(group_id, user_id);
    if (user_role < 2) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":12,"code":0,"msg":"无权限"})");
        return;
    }

    // 2. 检查申请人是否仍处于待验证状态
    int applicant_role = m_group_model.getRole(group_id, applicant_id);
    if (applicant_role != 0) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":12,"code":0,"msg":"该申请已被处理"})");
        return;
    }

    // 3. 处理申请
    if (!m_group_model.processGroupRequest(group_id, applicant_id, agree)) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":12,"code":0,"msg":"操作失败"})");
        return;
    }

    // 4. 通知申请人
    json notify;
    notify["type"] = 12;
    notify["code"] = 2;
    notify["group_id"] = group_id;
    notify["agree"] = agree;
    notify["msg"] = agree ? "管理员同意入群申请" : "管理员拒绝入群申请";

    Connection* user_conn = nullptr;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_user_conn.find(applicant_id);
        if (it != m_user_conn.end()) {
            user_conn = it->second;
        }
    }
    if (user_conn) {
        user_conn->getReactor()->handleWrite(user_conn, notify.dump());
    } else {
        m_redis.lpush("offline_msg:" + std::to_string(applicant_id),
                      notify.dump());
    }

    conn->getReactor()->handleWrite(conn,
                                    R"({"type":12,"code":1,"msg":"操作成功"})");
}

void ChatService::queryMembers(Connection* conn, const json& js) {
    int group_id = js["group_id"];
    int user_id = conn->getUserId();
    if (m_group_model.getRole(group_id, user_id) <= 0) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":13,"code":0,"msg":"不在该群中"})");
        return;
    }
    std::vector<int> members = m_group_model.queryMembers(group_id);
    json response;
    response["type"] = 13;
    response["code"] = 1;
    response["msg"] = members;
    conn->getReactor()->handleWrite(conn, response.dump());
}

void ChatService::quitGroup(Connection* conn, const json& js) {
    int group_id = js["group_id"];
    int user_id = conn->getUserId();
    int role = m_group_model.getRole(group_id, user_id);
    if (role == -1) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":14,"code":0,"msg":"不在该群中"})");
        return;
    }
    if (role == 3) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":14,"code":0,"msg":"群主不能直接退出"})");
        return;
    }
    if (!m_group_model.delMember(group_id, user_id)) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":14,"code":0,"msg":"退出失败"})");
        return;
    }
    conn->getReactor()->handleWrite(
        conn, R"({"type":14,"code":1,"msg":"已退出群聊"})");
}

void ChatService::dissolveGroup(Connection* conn, const json& js) {
    int group_id = js["group_id"];
    int user_id = conn->getUserId();
    if (m_group_model.getRole(group_id, user_id) != 3) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":15,"code":0,"msg":"仅群主可解散群聊"})");
        return;
    }
    if (!m_group_model.dissolveGroup(group_id)) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":15,"code":0,"msg":"解散失败"})");
        return;
    }
    conn->getReactor()->handleWrite(
        conn, R"({"type":15,"code":1,"msg":"已解散群聊"})");
}

void ChatService::promoteAdmin(Connection* conn, const json& js) {
    int group_id = js["group_id"];
    int target_id = js["id"];
    int user_id = conn->getUserId();
    if (m_group_model.getRole(group_id, user_id) != 3) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":16,"code":0,"msg":"仅群主可设置管理员"})");
        return;
    }
    if (m_group_model.getRole(group_id, target_id) != 1) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":16,"code":0,"msg":"目标成员状态异常"})");
        return;
    }
    if (!m_group_model.promoteAdmin(group_id, target_id)) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":16,"code":0,"msg":"操作失败"})");
        return;
    }
    conn->getReactor()->handleWrite(
        conn, R"({"type":16,"code":1,"msg":"成功设为管理员"})");
}

void ChatService::demoteAdmin(Connection* conn, const json& js) {
    int group_id = js["group_id"];
    int target_id = js["id"];
    int user_id = conn->getUserId();
    if (m_group_model.getRole(group_id, user_id) != 3) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":17,"code":0,"msg":"仅群主可撤销管理员"})");
        return;
    }
    if (m_group_model.getRole(group_id, target_id) != 2) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":17,"code":0,"msg":"目标并非管理员"})");
        return;
    }
    if (!m_group_model.demoteAdmin(group_id, target_id)) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":17,"code":0,"msg":"操作失败"})");
        return;
    }
    conn->getReactor()->handleWrite(
        conn, R"({"type":17,"code":1,"msg":"已撤销管理员"})");
}

void ChatService::kickMember(Connection* conn, const json& js) {
    int group_id = js["group_id"];
    int target_id = js["id"];
    int user_id = conn->getUserId();
    if (target_id == user_id) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":18,"code":0,"msg":"不能踢自己"})");
        return;
    }
    int user_role = m_group_model.getRole(group_id, user_id);
    int target_role = m_group_model.getRole(group_id, target_id);
    if (target_role == -1) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":18,"code":0,"msg":"目标不在群中"})");
        return;
    }
    if (target_role == 3) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":18,"code":0,"msg":"不能踢群主"})");
        return;
    }
    if (user_role == 2 && target_role >= 2) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":18,"code":0,"msg":"管理员不能踢管理员"})");
        return;
    }
    if (user_role < 2) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":18,"code":0,"msg":"无权限"})");
        return;
    }
    if (!m_group_model.delMember(group_id, target_id)) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":18,"code":0,"msg":"操作失败"})");
        return;
    }
    conn->getReactor()->handleWrite(
        conn, R"({"type":18,"code":1,"msg":"已踢出成员"})");
}

void ChatService::groupChat(Connection* conn, const json& js) {
    int group_id = js["group_id"];
    int user_id = conn->getUserId();
    std::string msg = js["msg"];
    if (m_group_model.getRole(group_id, user_id) <= 0) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":19,"code":0,"msg":"不在该群中"})");
        return;
    }
    if (!m_message_model.insert(user_id, group_id, 1, msg)) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":19,"code":0,"msg":"消息发送失败"})");
        return;
    }
    std::vector<int> members = m_group_model.queryMembers(group_id);
    json response;
    response["type"] = 19;
    response["code"] = 1;
    response["group_id"] = group_id;
    response["sender_id"] = user_id;
    response["msg"] = msg;
    for (int member_id : members) {
        if (member_id == user_id) {
            continue;
        }
        Connection* member_conn = nullptr;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto it = m_user_conn.find(member_id);
            if (it != m_user_conn.end()) {
                member_conn = it->second;
            }
        }
        if (member_conn) {
            member_conn->getReactor()->handleWrite(member_conn,
                                                   response.dump());
        } else {
            m_redis.lpush("offline_group_msg:" + std::to_string(member_id),
                          response.dump());
        }
    }
}

void ChatService::queryGroupHistory(Connection* conn, const json& js) {
    int group_id = js["group_id"];
    int user_id = conn->getUserId();
    if (m_group_model.getRole(group_id, user_id) <= 0) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":20,"code":0,"msg":"不在该群中"})");
        return;
    }
    std::vector<json> history = m_message_model.queryGroupHistory(group_id);
    json response;
    response["type"] = 20;
    response["code"] = 1;
    response["msg"] = history;
    conn->getReactor()->handleWrite(conn, response.dump());
}
