#include "EmailSender.h"

#include <curl/curl.h>
#include <cstring>
#include <sstream>
#include <thread>

std::once_flag EmailSender::s_init_flag;
const std::string EmailSender::SENDER = "3346099791@qq.com";
const std::string EmailSender::AUTH_CODE = "cdtlwyevhirvciha";
const std::string EmailSender::SMTP_URL = "smtps://smtp.qq.com:465";

struct UploadContext {
    const char* data;
    size_t size;
    size_t pos;
};

EmailSender::EmailSender() {
    std::call_once(s_init_flag,
                   []() { curl_global_init(CURL_GLOBAL_DEFAULT); });
}

bool EmailSender::sendCode(const std::string& to, const std::string& code) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        return false;
    }

    struct curl_slist* recipients = nullptr;
    recipients = curl_slist_append(recipients, to.c_str());
    std::string mail_text = "From: " + SENDER + "\r\nTo: " + to +
                            "\r\nSubject: ChatRoom 验证码\r\n\r\n" + code +
                            "\r\n";
    UploadContext ctx = {mail_text.data(), mail_text.size(), 0};
    curl_easy_setopt(curl, CURLOPT_URL, SMTP_URL.c_str());
    curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);
    curl_easy_setopt(curl, CURLOPT_USERNAME, SENDER.c_str());
    curl_easy_setopt(curl, CURLOPT_PASSWORD, AUTH_CODE.c_str());
    curl_easy_setopt(curl, CURLOPT_MAIL_FROM, SENDER.c_str());
    curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);
    curl_easy_setopt(
        curl, CURLOPT_READFUNCTION,
        +[](char* ptr, size_t size, size_t nmemb, void* userdata) -> size_t {
            auto* ctx = static_cast<UploadContext*>(userdata);
            size_t remaining = ctx->size - ctx->pos;
            size_t to_copy = size * nmemb;
            if (to_copy > remaining) {
                to_copy = remaining;
            }
            if (to_copy == 0) {
                return 0;
            }
            memcpy(ptr, ctx->data + ctx->pos, to_copy);
            ctx->pos += to_copy;
            return to_copy;
        });
    curl_easy_setopt(curl, CURLOPT_READDATA, &ctx);
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
    curl_easy_setopt(curl, CURLOPT_MAIL_AUTH, SENDER.c_str());
    curl_easy_setopt(curl, CURLOPT_LOGIN_OPTIONS, "AUTH=LOGIN");

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(recipients);
    curl_easy_cleanup(curl);

    return res == CURLE_OK;
}
