#pragma once

#include <functional>
#include <mutex>
#include <nlohmann/json.hpp>
#include <unordered_map>

#include "../model/FriendModel.h"
#include "../model/UserModel.h"
#include "../net/Connection.h"

using json = nlohmann::json;

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
    void addFriend(Connection* conn, const json& js);

   private:
    using handler = std::function<void(Connection*, const json&)>;
    std::unordered_map<int, handler> m_handlers;
    UserModel m_user_model;
    FriendModel m_friend_odel;
    std::unordered_map<int, Connection*> m_user_conn;
    std::mutex m_mutex;
};