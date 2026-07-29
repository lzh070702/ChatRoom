#include <hiredis/hiredis.h>
#include <string>

class Redis {
   public:
    Redis();
    // bool set(const std::string& key, const std::string& value);
    // bool get(const std::string& key, std::string& value);
    // bool del(const std::string& key);
    bool lpush(const std::string& key, const std::string& value);
    bool rpop(const std::string& key, std::string& value);

   private:
    redisContext* m_conn;
};