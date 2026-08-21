#pragma once

#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <nlohmann/json.hpp>
#include <unordered_map>

#include "../database/MySQLPool.h"
#include "../database/Redis.h"
#include "../email/EmailSender.h"
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
    void handle(std::shared_ptr<Connection> conn, const json& js);
    void logout(std::shared_ptr<Connection> conn);

   private:
    ChatService();
    ~ChatService();

    void heartbeat(std::shared_ptr<Connection> conn, const json& js);
    void signUp(std::shared_ptr<Connection> conn, const json& js);
    void signIn(std::shared_ptr<Connection> conn, const json& js);
    void sendCode(std::shared_ptr<Connection> conn, const json& js);
    void codeLogin(std::shared_ptr<Connection> conn, const json& js);
    void changePassword(std::shared_ptr<Connection> conn, const json& js);
    void exitLogin(std::shared_ptr<Connection> conn, const json& js);
    void signOut(std::shared_ptr<Connection> conn, const json& js);
    void queryFriends(std::shared_ptr<Connection> conn, const json& js);
    void sendRequest(std::shared_ptr<Connection> conn, const json& js);
    void processRequest(std::shared_ptr<Connection> conn, const json& js);
    void blockFriend(std::shared_ptr<Connection> conn, const json& js);
    void deleteFriend(std::shared_ptr<Connection> conn, const json& js);
    void oneChat(std::shared_ptr<Connection> conn, const json& js);
    void queryHistory(std::shared_ptr<Connection> conn, const json& js);
    void queryGroups(std::shared_ptr<Connection> conn, const json& js);
    void createGroup(std::shared_ptr<Connection> conn, const json& js);
    void inviteToGroup(std::shared_ptr<Connection> conn, const json& js);
    void applyGroups(std::shared_ptr<Connection> conn, const json& js);
    void processGroupRequest(std::shared_ptr<Connection> conn, const json& js);
    void queryMembers(std::shared_ptr<Connection> conn, const json& js);
    void setAdmin(std::shared_ptr<Connection> conn, const json& js);
    void kickMember(std::shared_ptr<Connection> conn, const json& js);
    void quitGroup(std::shared_ptr<Connection> conn, const json& js);
    void dissolveGroup(std::shared_ptr<Connection> conn, const json& js);
    void groupChat(std::shared_ptr<Connection> conn, const json& js);
    void queryGroupHistory(std::shared_ptr<Connection> conn, const json& js);
    void pullFile(std::shared_ptr<Connection> conn, const json& js);

    std::string base64Encode(const std::string& data);
    std::string base64Decode(const std::string& encoded);
    void saveFile(int message_id,
                  const std::string& file_name,
                  const std::string& file_data);
    std::string basename(const std::string& path);
    std::string sha256(const std::string& input);

   private:
    using handler =
        std::function<void(std::shared_ptr<Connection>, const json&)>;
    std::unordered_map<int, handler> m_handlers;

    Redis m_redis;
    MySQLPool m_mysql_pool;
    UserModel m_user_model;
    FriendModel m_friend_model;
    GroupModel m_group_model;
    MessageModel m_message_model;
    EmailSender m_email_sender;
    std::unordered_map<int, std::weak_ptr<Connection>> m_user_conn;
    std::mutex m_mutex;
    std::unordered_map<std::string, std::string> m_codes;
    std::mutex m_code_mutex;

    std::thread m_db_thread;
    std::deque<std::function<void()>> m_db_tasks;
    std::mutex m_db_mtx;
    std::condition_variable m_db_cv;
    std::atomic<bool> m_db_stop{false};
};