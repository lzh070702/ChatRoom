#pragma once

#include <mutex>
#include <string>

class EmailSender {
   public:
    EmailSender();
    bool sendCode(const std::string& to, const std::string& code);

   private:
    static std::once_flag s_init_flag;
    static const std::string SENDER;
    static const std::string AUTH_CODE;
    static const std::string SMTP_URL;
};
