#include <glog/logging.h>

#include "Redis.h"

Redis::Redis() {
    m_conn = redisConnect("127.0.0.1", 6379);
}

bool Redis::lpush(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(m_mutex);
    redisReply* reply = (redisReply*)redisCommand(m_conn, "LPUSH %s %s",
                                                  key.c_str(), value.c_str());
    if (reply == nullptr) {
        LOG(ERROR) << "Redis lpush failed: " << m_conn->errstr;
        return false;
    }
    bool success = (reply->type == REDIS_REPLY_INTEGER);
    freeReplyObject(reply);
    return success;
}

bool Redis::rpop(const std::string& key, std::string& value) {
    std::lock_guard<std::mutex> lock(m_mutex);
    redisReply* reply =
        (redisReply*)redisCommand(m_conn, "RPOP %s", key.c_str());
    if (reply == nullptr) {
        LOG(ERROR) << "Redis rpop failed: " << m_conn->errstr;
        return false;
    }
    bool success = (reply->type == REDIS_REPLY_STRING);
    if (success) {
        value = reply->str;
    }
    freeReplyObject(reply);
    return success;
}