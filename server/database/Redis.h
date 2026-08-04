#include <hiredis/hiredis.h>
#include <string>

class Redis {
   public:
    Redis();
    bool lpush(const std::string& key, const std::string& value);
    bool rpop(const std::string& key, std::string& value);

   private:
    redisContext* m_conn;
};