#pragma once
#include <functional>
#include <unordered_map>
#include "../net/Connection.h"
#include "../protocol/JsonProtocol.h"

class ChatService {
   public:
    ChatService(const ChatService&) = delete;
    ChatService& operator=(const ChatService&) = delete;

    static ChatService& instance();
    void handle(Connection* conn, const json& js);

   private:
    ChatService();

    void signIn(Connection* conn, const json& js);
    void signUp(Connection* conn, const json& js);

   private:
    using handler = std::function<void(Connection*, const json&)>;
    std::unordered_map<int, handler> m_handlers;
};