#pragma once

#include <functional>
#include <mutex>
#include <nlohmann/json.hpp>
#include <unordered_map>

#include "../database/Redis.h"
#include "../model/FriendModel.h"
#include "../model/GroupModel.h"
#include "../model/MessageModel.h"
#include "../model/UserModel.h"
#include "../net/Connection.h"

using json = nlohmann::json;

class Connection;

class ChatService {
   public:
    ChatService(const ChatService&) = delete;
    ChatService& operator=(const ChatService&) = delete;

    static ChatService& instance();
    void handle(Connection* conn, const json& js);
    // 登出
    void logout(Connection* conn);

   private:
    ChatService();

    void signIn(Connection* conn, const json& js);
    void signUp(Connection* conn, const json& js);
    void sendRequest(Connection* conn, const json& js);
    void processRequest(Connection* conn, const json& js);
    void queryFriends(Connection* conn, const json& js);
    void deleteFriend(Connection* conn, const json& js);
    void oneChat(Connection* conn, const json& js);
    void queryHistory(Connection* conn, const json& js);
    void createGroup(Connection* conn, const json& js);

   private:
    using handler = std::function<void(Connection*, const json&)>;
    std::unordered_map<int, handler> m_handlers;
    Redis m_redis;
    UserModel m_user_model;
    FriendModel m_friend_model;
    GroupModel m_group_model;
    MessageModel m_message_model;
    std::unordered_map<int, Connection*> m_user_conn;
    std::mutex m_mutex;
};