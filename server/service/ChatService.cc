#include <glog/logging.h>
#include <openssl/sha.h>
#include <fstream>
#include <iomanip>
#include <random>
#include <sstream>
#include <vector>

#include "ChatService.h"

ChatService& ChatService::instance() {
    static ChatService service;
    return service;
}

void ChatService::handle(std::shared_ptr<Connection> conn, const json& js) {
    int type = js["type"];
    auto it = m_handlers.find(type);
    if (it != m_handlers.end()) {
        it->second(conn, js);
    } else {
        conn->getReactor()->handleWrite(
            conn, R"({"type":0,"code":0,"msg":"异常请求"})");
        LOG(ERROR) << "Invalid request type=" << type;
        return;
    }
}

void ChatService::logout(std::shared_ptr<Connection> conn) {
    int id = conn->getUserId();
    if (id == -1) {
        return;
    }
    m_user_model.updateState(id, 0);
    std::lock_guard<std::mutex> lock(m_mutex);
    m_user_conn.erase(id);
}

ChatService::ChatService()
    : m_user_model(&m_mysql_pool),
      m_friend_model(&m_mysql_pool),
      m_group_model(&m_mysql_pool),
      m_message_model(&m_mysql_pool) {
    m_mysql_pool.init(4, "chatserver", "123456", "chatroom", "127.0.0.1");
    m_handlers[0] = [this](std::shared_ptr<Connection> c, const json& j) {
        heartbeat(c, j);
    };
    m_handlers[1] = [this](std::shared_ptr<Connection> c, const json& j) {
        signUp(c, j);
    };
    m_handlers[2] = [this](std::shared_ptr<Connection> c, const json& j) {
        signIn(c, j);
    };
    m_handlers[3] = [this](std::shared_ptr<Connection> c, const json& j) {
        sendCode(c, j);
    };
    m_handlers[4] = [this](std::shared_ptr<Connection> c, const json& j) {
        codeLogin(c, j);
    };
    m_handlers[5] = [this](std::shared_ptr<Connection> c, const json& j) {
        changePassword(c, j);
    };
    m_handlers[6] = [this](std::shared_ptr<Connection> c, const json& j) {
        exitLogin(c, j);
    };
    m_handlers[7] = [this](std::shared_ptr<Connection> c, const json& j) {
        signOut(c, j);
    };
    m_handlers[8] = [this](std::shared_ptr<Connection> c, const json& j) {
        queryFriends(c, j);
    };
    m_handlers[9] = [this](std::shared_ptr<Connection> c, const json& j) {
        sendRequest(c, j);
    };
    m_handlers[10] = [this](std::shared_ptr<Connection> c, const json& j) {
        processRequest(c, j);
    };
    m_handlers[11] = [this](std::shared_ptr<Connection> c, const json& j) {
        blockFriend(c, j);
    };
    m_handlers[12] = [this](std::shared_ptr<Connection> c, const json& j) {
        deleteFriend(c, j);
    };
    m_handlers[13] = [this](std::shared_ptr<Connection> c, const json& j) {
        oneChat(c, j);
    };
    m_handlers[14] = [this](std::shared_ptr<Connection> c, const json& j) {
        queryHistory(c, j);
    };
    m_handlers[15] = [this](std::shared_ptr<Connection> c, const json& j) {
        queryGroups(c, j);
    };
    m_handlers[16] = [this](std::shared_ptr<Connection> c, const json& j) {
        createGroup(c, j);
    };
    m_handlers[17] = [this](std::shared_ptr<Connection> c, const json& j) {
        inviteToGroup(c, j);
    };
    m_handlers[18] = [this](std::shared_ptr<Connection> c, const json& j) {
        applyGroups(c, j);
    };
    m_handlers[19] = [this](std::shared_ptr<Connection> c, const json& j) {
        processGroupRequest(c, j);
    };
    m_handlers[20] = [this](std::shared_ptr<Connection> c, const json& j) {
        queryMembers(c, j);
    };
    m_handlers[21] = [this](std::shared_ptr<Connection> c, const json& j) {
        setAdmin(c, j);
    };
    m_handlers[22] = [this](std::shared_ptr<Connection> c, const json& j) {
        kickMember(c, j);
    };
    m_handlers[23] = [this](std::shared_ptr<Connection> c, const json& j) {
        quitGroup(c, j);
    };
    m_handlers[24] = [this](std::shared_ptr<Connection> c, const json& j) {
        dissolveGroup(c, j);
    };
    m_handlers[25] = [this](std::shared_ptr<Connection> c, const json& j) {
        groupChat(c, j);
    };
    m_handlers[26] = [this](std::shared_ptr<Connection> c, const json& j) {
        queryGroupHistory(c, j);
    };
    m_handlers[27] = [this](std::shared_ptr<Connection> c, const json& j) {
        pullFile(c, j);
    };
}

void ChatService::heartbeat(std::shared_ptr<Connection> conn, const json& js) {
    conn->getReactor()->handleWrite(conn, R"({"type":0})");
}

void ChatService::signUp(std::shared_ptr<Connection> conn, const json& js) {
    std::string email = js["email"];
    User user;
    if (m_user_model.queryByEmail(email, user)) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":1,"code":0,"msg":"该邮箱已存在"})");
        return;
    }
    user.setName(js["name"]);
    user.setEmail(email);
    user.setPassword(sha256(js["password"]));
    user.setState(0);
    if (m_user_model.insert(user)) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":1,"code":1,"msg":"用户信息创建成功"})");
    } else {
        conn->getReactor()->handleWrite(
            conn, R"({"type":1,"code":0,"msg":"用户信息创建失败"})");
    }
}

void ChatService::signIn(std::shared_ptr<Connection> conn, const json& js) {
    std::string email = js["email"];
    User user;
    if (!m_user_model.queryByEmail(email, user)) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":2,"code":0,"msg":"该账号不存在"})");
        return;
    }
    if (user.getPassword() != sha256(js["password"])) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":2,"code":0,"msg":"密码错误"})");
        return;
    }
    if (user.getState() == 1) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":2,"code":0,"msg":"该账号已在其他设备登录"})");
        return;
    }
    if (!m_user_model.updateState(user.getId(), 1)) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":2,"code":0,"msg":"登录失败"})");
        return;
    }
    conn->setUserId(user.getId());
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_user_conn[user.getId()] = conn;
    }
    json response;
    response["type"] = 2;
    response["code"] = 1;
    response["msg"] = "登录成功";
    response["id"] = user.getId();
    response["name"] = user.getName();
    response["email"] = user.getEmail();
    conn->getReactor()->handleWrite(conn, response.dump());
    LOG(INFO) << "SignIn success uid=" << user.getId()
              << " email=" << user.getEmail();
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

void ChatService::sendCode(std::shared_ptr<Connection> conn, const json& js) {
    std::string email = js["email"];
    if (email.empty()) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":3,"code":0,"msg":"邮箱不能为空"})");
        return;
    }
    std::string code = []() -> std::string {
        std::mt19937 gen(std::random_device{}());
        std::uniform_int_distribution<int> dist(0, 999999);
        std::ostringstream oss;
        oss << std::setw(6) << std::setfill('0') << dist(gen);
        return oss.str();
    }();
    {
        std::lock_guard<std::mutex> lock(m_code_mutex);
        m_codes[email] = code;
    }
    if (!m_email_sender.sendCode(email, code)) {
        std::lock_guard<std::mutex> lock(m_code_mutex);
        m_codes.erase(email);
        conn->getReactor()->handleWrite(
            conn, R"({"type":3,"code":0,"msg":"邮件发送失败"})");
        return;
    }
    conn->getReactor()->handleWrite(
        conn, R"({"type":3,"code":1,"msg":"验证码已发送"})");
}

void ChatService::codeLogin(std::shared_ptr<Connection> conn, const json& js) {
    std::string email = js["email"];
    std::string code = js["code"];
    if (email.empty() || code.empty()) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":4,"code":0,"msg":"邮箱或验证码不能为空"})");
        return;
    }
    {
        std::lock_guard<std::mutex> lock(m_code_mutex);
        auto it = m_codes.find(email);
        if (it == m_codes.end()) {
            conn->getReactor()->handleWrite(
                conn, R"({"type":4,"code":0,"msg":"请先获取验证码"})");
            return;
        }
        if (it->second != code) {
            conn->getReactor()->handleWrite(
                conn, R"({"type":4,"code":0,"msg":"验证码错误"})");
            return;
        }
        m_codes.erase(it);
    }
    User user;
    if (m_user_model.queryByEmail(email, user)) {
        if (!m_user_model.updateState(user.getId(), 1)) {
            conn->getReactor()->handleWrite(
                conn, R"({"type":4,"code":0,"msg":"登录失败"})");
            return;
        }
        conn->setUserId(user.getId());
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_user_conn[user.getId()] = conn;
        }
        json response;
        response["type"] = 4;
        response["code"] = 1;
        response["msg"] = "登录成功";
        response["id"] = user.getId();
        response["name"] = user.getName();
        response["email"] = user.getEmail();
        conn->getReactor()->handleWrite(conn, response.dump());
        std::string data;
        std::string request_key =
            "offline_request:" + std::to_string(user.getId());
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
        LOG(INFO) << "CodeLogin success uid=" << user.getId()
                  << " email=" << email;
    } else {
        conn->getReactor()->handleWrite(
            conn, R"({"type":4,"code":0,"msg":"该账号不存在"})");
    }
}

void ChatService::changePassword(std::shared_ptr<Connection> conn,
                                 const json& js) {
    std::string email = js["email"];
    std::string code = js["code"];
    std::string password = js["password"];
    if (email.empty() || code.empty() || password.empty()) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":5,"code":0,"msg":"邮箱、验证码或密码不能为空"})");
        return;
    }
    {
        std::lock_guard<std::mutex> lock(m_code_mutex);
        auto it = m_codes.find(email);
        if (it == m_codes.end()) {
            conn->getReactor()->handleWrite(
                conn, R"({"type":5,"code":0,"msg":"请先获取验证码"})");
            return;
        }
        if (it->second != code) {
            conn->getReactor()->handleWrite(
                conn, R"({"type":5,"code":0,"msg":"验证码错误"})");
            return;
        }
        m_codes.erase(it);
    }
    User user;
    if (!m_user_model.queryByEmail(email, user)) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":5,"code":0,"msg":"邮箱未注册"})");
        return;
    }
    if (!m_user_model.updatePassword(user.getId(), sha256(password))) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":5,"code":0,"msg":"密码修改失败"})");
        return;
    }
    conn->getReactor()->handleWrite(
        conn, R"({"type":5,"code":1,"msg":"密码已修改"})");
}

void ChatService::exitLogin(std::shared_ptr<Connection> conn, const json& js) {
    int user_id = conn->getUserId();
    if (user_id == -1) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":6,"code":0,"msg":"未登录"})");
        return;
    }
    conn->getReactor()->handleWrite(
        conn, R"({"type":6,"code":1,"msg":"已退出登录"})");
    LOG(INFO) << "ExitLogin uid=" << user_id;
    logout(conn);
    conn->setUserId(-1);
}

void ChatService::signOut(std::shared_ptr<Connection> conn, const json& js) {
    int user_id = conn->getUserId();
    if (user_id == -1) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":7,"code":0,"msg":"未登录"})");
        return;
    }
    m_friend_model.removeAll(user_id);
    m_group_model.removeFromAll(user_id);
    m_message_model.removeAll(user_id);
    m_user_model.remove(user_id);
    conn->getReactor()->handleWrite(conn,
                                    R"({"type":7,"code":1,"msg":"已注销"})");
    LOG(INFO) << "SignOut uid=" << user_id;
    logout(conn);
    conn->setUserId(-1);
}

void ChatService::queryFriends(std::shared_ptr<Connection> conn,
                               const json& js) {
    int user_id = conn->getUserId();
    std::vector<User> friends = m_friend_model.queryFriends(conn->getUserId());
    json response;
    response["type"] = 8;
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

void ChatService::sendRequest(std::shared_ptr<Connection> conn,
                              const json& js) {
    int user_id = conn->getUserId();
    std::string email = js["email"];
    std::string my_email = js["my_email"];
    std::string name = js["name"];
    User user;
    if (!m_user_model.queryByEmail(email, user)) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":9,"code":0,"msg":"该用户不存在"})");
        return;
    }
    int friend_id = user.getId();
    if (friend_id == user_id) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":9,"code":0,"msg":"不能添加自己为好友"})");
        return;
    }
    if (m_friend_model.isFriend(user_id, friend_id) != -1) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":9,"code":0,"msg":"请勿重复添加"})");
        return;
    }
    if (!m_friend_model.insert(user_id, friend_id)) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":9,"code":0,"msg":"好友申请发送失败"})");
        return;
    }
    json response;
    response["type"] = 9;
    response["code"] = 2;
    response["msg"] = "发来好友申请";
    response["id"] = user_id;
    response["name"] = name;
    response["email"] = my_email;
    std::shared_ptr<Connection> friend_conn;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_user_conn.find(friend_id);
        if (it != m_user_conn.end()) {
            friend_conn = it->second.lock();
        }
    }
    if (friend_conn) {
        friend_conn->getReactor()->post([friend_conn, data = response.dump()] {
            friend_conn->sendData(data + '\n');
        });
        conn->getReactor()->handleWrite(
            conn, R"({"type":9,"code":1,"msg":"成功发送申请"})");
        return;
    }
    if (!m_redis.lpush("offline_request:" + std::to_string(friend_id),
                       response.dump())) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":9,"code":0,"msg":"好友申请发送失败"})");
        return;
    }
    conn->getReactor()->handleWrite(
        conn, R"({"type":9,"code":1,"msg":"成功发送申请"})");
}

void ChatService::processRequest(std::shared_ptr<Connection> conn,
                                 const json& js) {
    int user_id = conn->getUserId();
    int agree = js["agree"];
    int friend_id = js["id"];
    User user;
    if (!m_user_model.queryById(friend_id, user)) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":10,"code":0,"msg":"该用户已注销"})");
        return;
    }
    if (agree == 1) {
        m_friend_model.addFriend(user_id, friend_id);
        conn->getReactor()->handleWrite(
            conn, R"({"type":10,"code":1,"msg":"成功添加为好友"})");
        return;
    }
    m_friend_model.deleteFriend(user_id, friend_id);
    conn->getReactor()->handleWrite(
        conn, R"({"type":10,"code":1,"msg":"已拒绝该申请"})");
}

void ChatService::blockFriend(std::shared_ptr<Connection> conn,
                              const json& js) {
    int user_id = conn->getUserId();
    int friend_id = js["id"];
    bool block = js["block"];
    int status = m_friend_model.isFriend(user_id, friend_id);
    if (status != 2 && status != 3) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":11,"code":0,"msg":"操作失败"})");
        return;
    }
    int new_status = block ? 3 : 2;
    if (!m_friend_model.updateStatus(user_id, friend_id, new_status)) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":11,"code":0,"msg":"操作失败"})");
        return;
    }
    std::string msg = block ? "已拉黑" : "已取消拉黑";
    json response;
    response["type"] = 11;
    response["code"] = 1;
    response["msg"] = msg;
    conn->getReactor()->handleWrite(conn, response.dump());
}

void ChatService::deleteFriend(std::shared_ptr<Connection> conn,
                               const json& js) {
    int user_id = conn->getUserId();
    int friend_id = js["id"];
    if (!m_friend_model.deleteFriend(user_id, friend_id)) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":12,"code":0,"msg":"删除好友失败"})");
        return;
    }
    conn->getReactor()->handleWrite(
        conn, R"({"type":12,"code":1,"msg":"成功删除该好友"})");
}

void ChatService::oneChat(std::shared_ptr<Connection> conn, const json& js) {
    int friend_id = js["id"];
    int user_id = conn->getUserId();
    bool is_file = js["msg_type"];
    std::string content = js["msg"];
    User user;
    if (!m_user_model.queryById(friend_id, user)) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":13,"code":0,"msg":"该用户已注销"})");
        return;
    }
    if (m_friend_model.isFriend(user_id, friend_id) < 2) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":13,"code":0,"msg":"不是好友，无法聊天"})");
        return;
    }
    if (m_friend_model.isFriend(friend_id, user_id) == 3) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":13,"code":0,"msg":"已被对方拉黑"})");
        return;
    }
    int msg_id =
        m_message_model.insert(user_id, friend_id, 0, content, is_file);
    if (msg_id == -1) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":13,"code":0,"msg":"消息发送失败"})");
        return;
    }
    if (is_file) {
        saveFile(msg_id, content, js["file_data"]);
        content = std::to_string(msg_id) + "_" + content;
        m_message_model.updateContent(msg_id, content);
    }
    User sender;
    m_user_model.queryById(user_id, sender);
    json response;
    response["type"] = 13;
    response["code"] = 2;
    response["id"] = user_id;
    response["name"] = sender.getName();
    response["msg"] = content;
    response["msg_type"] = js["msg_type"];
    std::shared_ptr<Connection> friend_conn;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_user_conn.find(friend_id);
        if (it != m_user_conn.end()) {
            friend_conn = it->second.lock();
        }
    }
    if (friend_conn) {
        friend_conn->getReactor()->post([friend_conn, data = response.dump()] {
            friend_conn->sendData(data + '\n');
        });
        conn->getReactor()->handleWrite(
            conn, R"({"type":13,"code":1,"msg":"发送成功"})");
        return;
    }
    if (!m_redis.lpush("offline_msg:" + std::to_string(friend_id),
                       response.dump())) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":13,"code":0,"msg":"消息发送失败"})");
        return;
    }
    conn->getReactor()->handleWrite(conn,
                                    R"({"type":13,"code":1,"msg":"发送成功"})");
}

void ChatService::queryHistory(std::shared_ptr<Connection> conn,
                               const json& js) {
    int friend_id = js["id"];
    int user_id = conn->getUserId();
    int scope = js.value("scope", 0);
    std::vector<json> history =
        m_message_model.queryHistory(user_id, friend_id, scope);
    json response;
    response["type"] = 14;
    response["code"] = 1;
    response["msg"] = history;
    conn->getReactor()->handleWrite(conn, response.dump());
}

void ChatService::queryGroups(std::shared_ptr<Connection> conn,
                              const json& js) {
    std::vector<json> groups = m_group_model.queryGroups(conn->getUserId());
    json response;
    response["type"] = 15;
    response["code"] = 1;
    response["msg"] = groups;
    conn->getReactor()->handleWrite(conn, response.dump());
}

void ChatService::createGroup(std::shared_ptr<Connection> conn,
                              const json& js) {
    std::string name = js["name"];
    int user_id = conn->getUserId();
    int group_id = 0;
    if (!m_group_model.createGroup(name, user_id, group_id)) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":16,"code":0,"msg":"创建群聊失败"})");
        return;
    }
    json response;
    response["type"] = 16;
    response["code"] = 1;
    response["msg"] = "成功创建群聊";
    response["group_id"] = group_id;
    conn->getReactor()->handleWrite(conn, response.dump());
}

void ChatService::inviteToGroup(std::shared_ptr<Connection> conn,
                                const json& js) {
    int group_id = js["group_id"];
    int friend_id = js["id"];
    int user_id = conn->getUserId();
    int user_role = m_group_model.getRole(group_id, user_id);
    if (user_role < 2) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":17,"code":0,"msg":"无权限"})");
        return;
    }
    if (m_friend_model.isFriend(user_id, friend_id) != 2) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":17,"code":0,"msg":"不是好友"})");
        return;
    }
    if (m_group_model.isInGroup(group_id, friend_id)) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":17,"code":0,"msg":"对方已在群中"})");
        return;
    }
    if (!m_group_model.addMember(group_id, friend_id, 1)) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":17,"code":0,"msg":"邀请失败"})");
        return;
    }
    json notify;
    notify["type"] = 17;
    notify["code"] = 2;
    notify["group_id"] = group_id;
    notify["group_name"] = m_group_model.getGroupName(group_id);
    notify["msg"] = "你已被邀请加入群聊";
    std::shared_ptr<Connection> friend_conn;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_user_conn.find(friend_id);
        if (it != m_user_conn.end()) {
            friend_conn = it->second.lock();
        }
    }
    if (friend_conn) {
        friend_conn->getReactor()->post([friend_conn, data = notify.dump()] {
            friend_conn->sendData(data + '\n');
        });
    } else {
        m_redis.lpush("offline_msg:" + std::to_string(friend_id),
                      notify.dump());
    }
    conn->getReactor()->handleWrite(conn,
                                    R"({"type":17,"code":1,"msg":"邀请成功"})");
}

void ChatService::applyGroups(std::shared_ptr<Connection> conn,
                              const json& js) {
    int group_id = js["group_id"];
    std::string email = js["email"];
    std::string name = js["name"];
    int user_id = conn->getUserId();
    if (!m_group_model.groupExist(group_id)) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":18,"code":0,"msg":"该群聊不存在"})");
        return;
    }
    if (m_group_model.isInGroup(group_id, user_id)) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":18,"code":0,"msg":"禁止重复加入该群"})");
        return;
    }
    std::vector<int> users;
    if (!m_group_model.applyGroup(group_id, user_id, users)) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":18,"code":0,"msg":"入群申请发送失败"})");
        return;
    }
    json response;
    response["type"] = 18;
    response["code"] = 2;
    response["msg"] = "有新的入群申请";
    response["id"] = user_id;
    response["name"] = name;
    response["email"] = email;
    for (int& user : users) {
        std::shared_ptr<Connection> user_conn;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto it = m_user_conn.find(user);
            if (it != m_user_conn.end()) {
                user_conn = it->second.lock();
            }
        }
        if (user_conn) {
            user_conn->getReactor()->post([user_conn, data = response.dump()] {
                user_conn->sendData(data + '\n');
            });
        } else {
            if (!m_redis.lpush("offline_group_request:" + std::to_string(user),
                               response.dump())) {
                conn->getReactor()->handleWrite(
                    conn, R"({"type":18,"code":0,"msg":"入群申请发送失败"})");
                return;
            }
        }
    }
    conn->getReactor()->handleWrite(
        conn, R"({"type":18,"code":1,"msg":"入群申请发送成功"})");
}

void ChatService::processGroupRequest(std::shared_ptr<Connection> conn,
                                      const json& js) {
    int group_id = js["group_id"];
    bool agree = js["agree"];
    int applicant_id = js["id"];
    int user_id = conn->getUserId();
    int user_role = m_group_model.getRole(group_id, user_id);
    if (user_role < 2) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":19,"code":0,"msg":"无权限"})");
        return;
    }
    int applicant_role = m_group_model.getRole(group_id, applicant_id);
    if (applicant_role != 0) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":19,"code":0,"msg":"该申请已被处理"})");
        return;
    }
    if (!m_group_model.processGroupRequest(group_id, applicant_id, agree)) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":19,"code":0,"msg":"操作失败"})");
        return;
    }
    json notify;
    notify["type"] = 19;
    notify["code"] = 2;
    notify["group_id"] = group_id;
    notify["agree"] = agree;
    notify["msg"] = agree ? "管理员同意入群申请" : "管理员拒绝入群申请";
    std::shared_ptr<Connection> user_conn;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_user_conn.find(applicant_id);
        if (it != m_user_conn.end()) {
            user_conn = it->second.lock();
        }
    }
    if (user_conn) {
        user_conn->getReactor()->post([user_conn, data = notify.dump()] {
            user_conn->sendData(data + '\n');
        });
    } else {
        m_redis.lpush("offline_msg:" + std::to_string(applicant_id),
                      notify.dump());
    }
    conn->getReactor()->handleWrite(conn,
                                    R"({"type":19,"code":1,"msg":"操作成功"})");
}

void ChatService::queryMembers(std::shared_ptr<Connection> conn,
                               const json& js) {
    int group_id = js["group_id"];
    int user_id = conn->getUserId();
    if (m_group_model.getRole(group_id, user_id) <= 0) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":20,"code":0,"msg":"不在该群中"})");
        return;
    }
    auto members = m_group_model.queryMembers(group_id);
    json response;
    response["type"] = 20;
    response["code"] = 1;
    for (auto& [member_id, role] : members) {
        json info;
        User user;
        m_user_model.queryById(member_id, user);
        info["id"] = user.getId();
        info["name"] = user.getName();
        info["email"] = user.getEmail();
        info["state"] = user.getState();
        info["role"] = role;
        response["members"].push_back(info);
    }
    conn->getReactor()->handleWrite(conn, response.dump());
}

void ChatService::setAdmin(std::shared_ptr<Connection> conn, const json& js) {
    int group_id = js["group_id"];
    int target_id = js["id"];
    bool admin = js["admin"];
    int user_id = conn->getUserId();
    if (m_group_model.getRole(group_id, user_id) != 3) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":21,"code":0,"msg":"仅群主可操作"})");
        return;
    }
    int target_role = m_group_model.getRole(group_id, target_id);
    if (admin && target_role != 1) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":21,"code":0,"msg":"目标成员状态异常"})");
        return;
    }
    if (!admin && target_role != 2) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":21,"code":0,"msg":"目标并非管理员"})");
        return;
    }
    int new_role = admin ? 2 : 1;
    if (!m_group_model.setRole(group_id, target_id, new_role)) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":21,"code":0,"msg":"操作失败"})");
        return;
    }
    std::string msg = admin ? "成功设为管理员" : "已撤销管理员";
    json response;
    response["type"] = 21;
    response["code"] = 1;
    response["msg"] = msg;
    conn->getReactor()->handleWrite(conn, response.dump());
}

void ChatService::kickMember(std::shared_ptr<Connection> conn, const json& js) {
    int group_id = js["group_id"];
    int target_id = js["id"];
    int user_id = conn->getUserId();
    if (target_id == user_id) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":22,"code":0,"msg":"不能踢自己"})");
        return;
    }
    int user_role = m_group_model.getRole(group_id, user_id);
    int target_role = m_group_model.getRole(group_id, target_id);
    if (target_role == -1) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":22,"code":0,"msg":"目标不在群中"})");
        return;
    }
    if (target_role == 3) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":22,"code":0,"msg":"不能踢群主"})");
        return;
    }
    if (user_role == 2 && target_role >= 2) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":22,"code":0,"msg":"管理员不能踢管理员"})");
        return;
    }
    if (user_role < 2) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":22,"code":0,"msg":"无权限"})");
        return;
    }
    if (!m_group_model.delMember(group_id, target_id)) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":22,"code":0,"msg":"操作失败"})");
        return;
    }
    conn->getReactor()->handleWrite(
        conn, R"({"type":22,"code":1,"msg":"已踢出成员"})");
}

void ChatService::quitGroup(std::shared_ptr<Connection> conn, const json& js) {
    int group_id = js["group_id"];
    int user_id = conn->getUserId();
    int role = m_group_model.getRole(group_id, user_id);
    if (role == -1) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":23,"code":0,"msg":"不在该群中"})");
        return;
    }
    if (role == 3) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":23,"code":0,"msg":"群主不能直接退出"})");
        return;
    }
    if (!m_group_model.delMember(group_id, user_id)) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":23,"code":0,"msg":"退出失败"})");
        return;
    }
    conn->getReactor()->handleWrite(
        conn, R"({"type":23,"code":1,"msg":"已退出群聊"})");
}

void ChatService::dissolveGroup(std::shared_ptr<Connection> conn,
                                const json& js) {
    int group_id = js["group_id"];
    int user_id = conn->getUserId();
    if (m_group_model.getRole(group_id, user_id) != 3) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":24,"code":0,"msg":"仅群主可解散群聊"})");
        return;
    }
    if (!m_group_model.dissolveGroup(group_id)) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":24,"code":0,"msg":"解散失败"})");
        return;
    }
    conn->getReactor()->handleWrite(
        conn, R"({"type":24,"code":1,"msg":"已解散群聊"})");
}

void ChatService::groupChat(std::shared_ptr<Connection> conn, const json& js) {
    int group_id = js["group_id"];
    int user_id = conn->getUserId();
    bool is_file = js["msg_type"];
    std::string content = js["msg"];
    if (m_group_model.getRole(group_id, user_id) <= 0) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":25,"code":0,"msg":"不在该群中"})");
        return;
    }
    int msg_id = m_message_model.insert(user_id, group_id, 1, content, is_file);
    if (msg_id == -1) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":25,"code":0,"msg":"消息发送失败"})");
        return;
    }
    if (is_file) {
        saveFile(msg_id, content, js["file_data"]);
        content = std::to_string(msg_id) + "_" + content;
        m_message_model.updateContent(msg_id, content);
    }
    auto members = m_group_model.queryMembers(group_id);
    json response;
    response["type"] = 25;
    response["code"] = 2;
    response["group_id"] = group_id;
    response["sender_id"] = user_id;
    response["group_name"] = m_group_model.getGroupName(group_id);
    response["msg"] = content;
    response["msg_type"] = js["msg_type"];
    for (auto& [member_id, role] : members) {
        if (member_id == user_id) {
            continue;
        }
        std::shared_ptr<Connection> member_conn;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto it = m_user_conn.find(member_id);
            if (it != m_user_conn.end()) {
                member_conn = it->second.lock();
            }
        }
        if (member_conn) {
            member_conn->getReactor()->post(
                [member_conn, data = response.dump()] {
                    member_conn->sendData(data + '\n');
                });
        } else {
            m_redis.lpush("offline_group_msg:" + std::to_string(member_id),
                          response.dump());
        }
    }
}

void ChatService::queryGroupHistory(std::shared_ptr<Connection> conn,
                                    const json& js) {
    int group_id = js["group_id"];
    int user_id = conn->getUserId();
    if (m_group_model.getRole(group_id, user_id) <= 0) {
        conn->getReactor()->handleWrite(
            conn, R"({"type":26,"code":0,"msg":"不在该群中"})");
        return;
    }
    int scope = js.value("scope", 0);
    std::vector<json> history =
        m_message_model.queryGroupHistory(group_id, scope);
    json response;
    response["type"] = 26;
    response["code"] = 1;
    response["msg"] = history;
    conn->getReactor()->handleWrite(conn, response.dump());
}

void ChatService::pullFile(std::shared_ptr<Connection> conn, const json& js) {
    std::string file_name = js["msg"];
    std::string path = "./files/" + file_name;
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) {
        json response;
        response["type"] = 27;
        response["code"] = 0;
        response["msg"] = "文件不存在";
        conn->getReactor()->handleWrite(conn, response.dump());
        return;
    }
    std::string data((std::istreambuf_iterator<char>(ifs)),
                     std::istreambuf_iterator<char>());
    json response;
    response["type"] = 27;
    response["code"] = 1;
    response["msg"] = file_name;
    response["file_data"] = base64Encode(data);
    conn->getReactor()->handleWrite(conn, response.dump());
}

void ChatService::saveFile(int message_id,
                           const std::string& file_name,
                           const std::string& file_data) {
    std::string path =
        "./files/" + std::to_string(message_id) + "_" + file_name;
    std::ofstream ofs(path, std::ios::binary);
    std::string decoded = base64Decode(file_data);
    ofs.write(decoded.data(), decoded.size());
}

std::string ChatService::base64Encode(const std::string& data) {
    static const char* table =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string encoded;
    int val = 0, bits = -6;
    for (unsigned char c : data) {
        val = (val << 8) + c;
        bits += 8;
        while (bits >= 0) {
            encoded.push_back(table[(val >> bits) & 0x3F]);
            bits -= 6;
        }
    }
    if (bits > -6) {
        encoded.push_back(table[((val << 6) >> bits) & 0x3F]);
    }
    while (encoded.size() % 4) {
        encoded.push_back('=');
    }
    return encoded;
}

std::string ChatService::base64Decode(const std::string& encoded) {
    static const char* table =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    int decode_table[256] = {};
    for (int i = 0; i < 64; i++) {
        decode_table[static_cast<int>(table[i])] = i;
    }
    std::string decoded;
    int val = 0, bits = -8;
    for (unsigned char c : encoded) {
        if (c == '=')
            break;
        if (decode_table[c] == 0 && c != 'A')
            continue;
        val = (val << 6) + decode_table[c];
        bits += 6;
        if (bits >= 0) {
            decoded.push_back(static_cast<char>((val >> bits) & 0xFF));
            bits -= 8;
        }
    }
    return decoded;
}

std::string ChatService::sha256(const std::string& input) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(input.c_str()), input.size(),
           hash);
    std::ostringstream oss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        oss << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<int>(hash[i]);
    }
    return oss.str();
}