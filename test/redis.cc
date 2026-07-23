#include <hiredis/hiredis.h>
#include <iostream>

int main() {
    redisContext* conn = redisConnect("127.0.0.1", 6379);
    if (conn == nullptr || conn->err) {
        std::cout << "连接失败" << std::endl;
        return 1;
    }
    std::cout << "连接成功" << std::endl;
    redisReply* reply = (redisReply*)redisCommand(conn, "SET name Alice");
    std::cout << reply->str << std::endl;
    freeReplyObject(reply);
    redisFree(conn);
    return 0;
}