#include <curl/curl.h>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <string>

using namespace std;
// ========== 邮件配置 ==========
const string RECIPIENT = "1729261590@qq.com";       // 收件人
const string SENDER = "3346099791@qq.com";          // 发件人
const string AUTH_CODE = "cdtlwyevhirvciha";        // QQ邮箱SMTP授权码
const string SMTP_URL = "smtps://smtp.qq.com:465";  // SMTP服务器地址
// =============================

// 回调：把邮件内容逐块喂给 libcurl
struct UploadContext {
    const char* data;
    size_t size;
    size_t pos;
};

int main() {
    string num = []() -> string {
        thread_local mt19937 gen(random_device{}());
        uniform_int_distribution<int> dist(0, 999999);
        ostringstream oss;
        oss << setw(6) << setfill('0') << dist(gen);
        return oss.str();
    }();
    curl_global_init(CURL_GLOBAL_DEFAULT);
    CURL* curl = curl_easy_init();
    if (!curl) {
        cerr << "curl_easy_init() 失败" << endl;
        return 1;
    }

    // 构造收件人列表
    struct curl_slist* recipients = nullptr;
    recipients = curl_slist_append(recipients, RECIPIENT.c_str());

    // 构造邮件内容（仅正文 "1"，无额外邮件头时 libcurl 会自动加上
    // From/To/Subject）
    const string mail_text = "From: " + SENDER + "\r\nTo: " + RECIPIENT +
                             "\r\nSubject: 验证码\r\n\r\n" + num + "\r\n";

    UploadContext ctx = {mail_text.data(), mail_text.size(), 0};

    // 设置 libcurl 选项
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
            if (to_copy > remaining)
                to_copy = remaining;
            if (to_copy == 0)
                return 0;
            memcpy(ptr, ctx->data + ctx->pos, to_copy);
            ctx->pos += to_copy;
            return to_copy;
        });
    curl_easy_setopt(curl, CURLOPT_READDATA, &ctx);
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
    // QQ 邮箱要求验证发件人与认证用户一致
    curl_easy_setopt(curl, CURLOPT_MAIL_AUTH, SENDER.c_str());
    // QQ 邮箱须使用 AUTH LOGIN 方式（PLAIN 方式会失败）
    curl_easy_setopt(curl, CURLOPT_LOGIN_OPTIONS, "AUTH=LOGIN");

    cout << "正在向 " << RECIPIENT << " 发送邮件..." << endl;
    CURLcode res = curl_easy_perform(curl);

    if (res == CURLE_OK) {
        cout << "✓ 邮件发送成功！" << endl;
        cout << "请输入验证码：";
        string s;
        cin >> s;
        if (s == num) {
            cout << "登录成功" << endl;
        } else {
            cout << "验证码错误" << endl;
        }
    } else {
        cerr << "✗ 发送失败: " << curl_easy_strerror(res) << endl;
    }
    curl_slist_free_all(recipients);
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    return 0;
}