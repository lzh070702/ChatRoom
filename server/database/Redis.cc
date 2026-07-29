#include "Redis.h"

Redis::Redis() {
    m_conn = redisConnect("127.0.0.1", 6379);
}

// bool Redis::set(const std::string& key, const std::string& value) {
//     redisReply* reply = (redisReply*)redisCommand(m_conn, "SET %s %s",
//                                                   key.c_str(), value.c_str());
//     if (reply == nullptr) {
//         return false;
//     }
//     bool success =
//         (reply->type == REDIS_REPLY_STATUS && std::string(reply->str) == "OK");
//     freeReplyObject(reply);
//     return success;
// }

// bool Redis::get(const std::string& key, std::string& value) {
//     redisReply* reply =
//         (redisReply*)redisCommand(m_conn, "GET %s", key.c_str());
//     if (reply == nullptr) {
//         return false;
//     }
//     if (reply->type == REDIS_REPLY_STRING) {
//         value = reply->str;
//         freeReplyObject(reply);
//         return true;
//     }
//     freeReplyObject(reply);
//     return false;
// }

// bool Redis::del(const std::string& key) {
//     redisReply* reply =
//         (redisReply*)redisCommand(m_conn, "DEL %s", key.c_str());
//     if (reply == nullptr) {
//         return false;
//     }
//     bool success = (reply->type == REDIS_REPLY_INTEGER);
//     freeReplyObject(reply);
//     return success;
// }

bool Redis::lpush(const std::string& key, const std::string& value) {
    redisReply* reply = (redisReply*)redisCommand(m_conn, "LPUSH %s %s",
                                                  key.c_str(), value.c_str());
    if (reply == nullptr) {
        return false;
    }
    bool success = (reply->type == REDIS_REPLY_INTEGER);
    freeReplyObject(reply);
    return success;
}

bool Redis::rpop(const std::string& key, std::string& value) {
    redisReply* reply =
        (redisReply*)redisCommand(m_conn, "RPOP %s", key.c_str());
    if (reply == nullptr) {
        return false;
    }
    bool success = (reply->type == REDIS_REPLY_STRING);
    if (success) {
        value = reply->str;
    }
    freeReplyObject(reply);
    return success;
}