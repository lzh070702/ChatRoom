#include <readline/readline.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <deque>
#include <iostream>
#include <mutex>
#include <nlohmann/json.hpp>
#include <queue>
#include <string>
#include <thread>

#include "net/TcpClient.h"
#include "pool.h"

using namespace std;
using json = nlohmann::json;

atomic<bool> g_running{true};

mutex g_opt_mtx;
queue<string> g_opt_que;
int g_opt_efd;

mutex g_ipt_mtx;
queue<string> g_ipt_queu;
condition_variable g_ipt_cv;

mutex g_rsp_mtx;
queue<json> g_rsp_queu;
condition_variable g_rsp_cv;

mutex g_ui_mutex;
string g_prefix;

void netLoop(TcpClient* client, pool* pool);
void ioLoop();
void on_line(char* line);
void parseOpt(const string& msg, chrono::steady_clock::time_point& last_recv);
void printOpt();
void pushOpt(const string& smsg);
vector<string> popOpt();
void pushIpt(const string& smsg);
string popIpt();
void pushRsp(const json& js);
json popRsp();
void setPrefix(const string& prefix);

void authOptions(TcpClient& client);
void signUp(TcpClient& client);
void passwordSignIn(TcpClient& client);
void codeSignIn(TcpClient& client);
void home(TcpClient& client, const json& user);
bool settings(TcpClient& client, const json& user);

int main(int argc, char* argv[]) {
    string host = (argc > 1) ? argv[1] : "127.0.0.1";
    int port = (argc > 2) ? stoi(argv[2]) : 8000;
    TcpClient client;
    if (!client.connectServer(host, port)) {
        cerr << "连接失败" << endl;
        return 1;
    }
    g_opt_efd = eventfd(0, EFD_NONBLOCK);

    pool pool(4);
    pool.enqueue(netLoop, &client, &pool);
    pool.enqueue(ioLoop);
    authOptions(client);
    return 0;
}

void netLoop(TcpClient* client, pool* pool) {
    int epfd = epoll_create1(0);
    epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.fd = client->getFd();
    epoll_ctl(epfd, EPOLL_CTL_ADD, client->getFd(), &ev);

    auto last_send = chrono::steady_clock::now();
    auto last_recv = chrono::steady_clock::now();
    epoll_event events[4];
    while (g_running) {
        int n = epoll_wait(epfd, events, 4, 500);
        auto now = chrono::steady_clock::now();
        if (now - last_send >= chrono::seconds(20)) {
            client->sendData(R"({"type":0})");
            last_send = now;
        }
        if (now - last_recv >= chrono::seconds(60)) {
            pushOpt("与服务器断开连接\n");
            g_running = false;
            g_ipt_cv.notify_all();
            g_rsp_cv.notify_all();
            break;
        }
        for (int i = 0; i < n; ++i) {
            if (events[i].data.fd != client->getFd()) {
                continue;
            }
            string msg = client->recvData();
            while (!msg.empty()) {
                parseOpt(msg, last_recv);
                msg = client->recvData();
            }
        }
    }
    close(epfd);
}

void ioLoop() {
    int epfd = epoll_create1(0);
    epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.fd = STDIN_FILENO;
    epoll_ctl(epfd, EPOLL_CTL_ADD, STDIN_FILENO, &ev);
    ev.data.fd = g_opt_efd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, g_opt_efd, &ev);
    rl_callback_handler_install("", on_line);
    epoll_event events[2];
    while (g_running) {
        int n = epoll_wait(epfd, events, 2, 100);
        if (n < 0) {
            if (errno == EINTR)
                continue;
            break;
        }
        for (int i = 0; i < n; i++) {
            if (events[i].data.fd == g_opt_efd) {
                uint64_t val;
                read(g_opt_efd, &val, sizeof(val));
                printOpt();
            } else if (events[i].data.fd == STDIN_FILENO) {
                rl_callback_read_char();
            }
        }
    }

    rl_callback_handler_remove();
    close(epfd);
}

void on_line(char* line) {
    if (line == nullptr) {
        g_running = false;
        g_ipt_cv.notify_all();
        g_rsp_cv.notify_all();
        return;
    }
    string prefix;
    {
        lock_guard<mutex> lk(g_ui_mutex);
        prefix = g_prefix;
    }
    if (*line) {
        fprintf(rl_outstream, "\033[A\033[A\033[K%s %s\n", prefix.c_str(),
                line);
        pushIpt(line);
    } else {
        fprintf(rl_outstream, "\033[A");
    }
    free(line);
    rl_callback_handler_remove();
    rl_callback_handler_install("", on_line);
}

void parseOpt(const string& msg, chrono::steady_clock::time_point& last_recv) {
    json js;
    try {
        js = json::parse(msg);
    } catch (...) {
        return;
    }
    int type = js["type"];
    if (type == 0) {
        last_recv = chrono::steady_clock::now();
        return;
    }
    int code = js["code"];
    if (code == 2) {
        string rsp;
        if (type == 8) {
            rsp = string(js["name"]) + string(js["msg"]);
        }
        if (type == 12) {
            rsp = "好友" + to_string(js["id"]) + "发来一条新消息";
        }
        if (type == 16) {
            rsp = string(js["msg"]) + string(js["group_name"]);
        }
        if (type == 17) {
            rsp = string(js["msg"]) + string(js["name"]);
        }
        if (type == 18) {
            rsp = string(js["msg"]);
        }
        if (type == 24) {
            rsp = "群聊" + to_string(js["group_id"]) + "发来一条新消息";
        }
        pushOpt(rsp + "\n");
        return;
    }
    pushRsp(js);
}

void printOpt() {
    auto msgs = popOpt();
    if (msgs.empty())
        return;

    int saved_point = rl_point;
    char* saved_line = rl_copy_text(0, rl_end);
    fprintf(rl_outstream, "\r\033[K");
    for (auto& m : msgs) {
        fprintf(rl_outstream, "%s", m.c_str());
    }
    rl_on_new_line();
    rl_replace_line(saved_line, 0);
    rl_point = saved_point;
    rl_redisplay();
    free(saved_line);
    fflush(rl_outstream);
}

void pushOpt(const string& smsg) {
    {
        lock_guard<mutex> lk(g_opt_mtx);
        g_opt_que.push(smsg);
    }
    uint64_t one = 1;
    write(g_opt_efd, &one, sizeof(one));
}

vector<string> popOpt() {
    lock_guard<mutex> lk(g_opt_mtx);
    vector<string> msgs;
    while (!g_opt_que.empty()) {
        msgs.push_back(g_opt_que.front());
        g_opt_que.pop();
    }
    return msgs;
}

void pushIpt(const string& smsg) {
    {
        lock_guard<mutex> lk(g_ipt_mtx);
        g_ipt_queu.push(smsg);
    }
    g_ipt_cv.notify_one();
}

string popIpt() {
    unique_lock<mutex> lk(g_ipt_mtx);
    g_ipt_cv.wait(lk, [] { return !g_ipt_queu.empty() || !g_running; });
    if (!g_running || g_ipt_queu.empty())
        return "";
    string s = g_ipt_queu.front();
    g_ipt_queu.pop();
    return s;
}

void pushRsp(const json& js) {
    {
        lock_guard<mutex> lk(g_rsp_mtx);
        g_rsp_queu.push(js);
    }
    g_rsp_cv.notify_one();
}

json popRsp() {
    unique_lock<mutex> lk(g_rsp_mtx);
    g_rsp_cv.wait(lk, [] { return !g_rsp_queu.empty() || !g_running; });
    if (!g_running || g_rsp_queu.empty())
        return json();
    json js = g_rsp_queu.front();
    g_rsp_queu.pop();
    return js;
}

void setPrefix(const string& prefix) {
    lock_guard<mutex> lk(g_ui_mutex);
    g_prefix = prefix;
}

void authOptions(TcpClient& client) {
    while (g_running) {
        system("clear");
        pushOpt("────── 登录页面 ──────\n");
        pushOpt("======================\n");
        pushOpt("1. 密码登录\n");
        pushOpt("2. 验证码登录\n");
        pushOpt("3. 注册\n");
        pushOpt("4. 退出\n");
        pushOpt("======================\n");
        pushOpt("请选择:\n");
        setPrefix("选择:");
        string input = popIpt();
        if (!g_running || input.empty())
            continue;

        if (input == "1") {
            passwordSignIn(client);
            continue;
        } else if (input == "2") {
            codeSignIn(client);
            continue;
        } else if (input == "3") {
            signUp(client);
            continue;
        } else if (input == "4") {
            g_running = false;
            g_ipt_cv.notify_all();
            g_rsp_cv.notify_all();
            return;
        }
    }
}

void signUp(TcpClient& client) {
    while (g_running) {
        system("clear");
        pushOpt("──────── 注册 ────────\n");
        pushOpt("======================\n");
        pushOpt("请输入qq邮箱:\n");
        setPrefix("qq邮箱:");
        string email = popIpt();
        pushOpt("请输入用户名:\n");
        setPrefix("用户名:");
        string name = popIpt();
        pushOpt("请输入密码:\n");
        setPrefix("密码:");
        string password = popIpt();
        pushOpt("======================\n");
        pushOpt("1. 注册    2. 返回\n");
        pushOpt("======================\n");
        pushOpt("请选择:\n");
        setPrefix("选择:");
        if (popIpt() == "1") {
            json req;
            req["type"] = 1;
            req["email"] = email;
            req["name"] = name;
            req["password"] = password;
            client.sendData(req.dump());
            json rsp = popRsp();
            if (rsp["code"] == 1) {
                pushOpt(string(rsp["msg"]) + "\n");
                return;
            } else {
                pushOpt("======================\n");
                pushOpt(string(rsp["msg"]) + "\n");
                pushOpt("1. 重新注册    2. 返回\n");
                pushOpt("======================\n");
                pushOpt("请选择:\n");
                setPrefix("选择:");
                if (popIpt() != "1") {
                    return;
                }
            }
        } else {
            return;
        }
    }
}

void passwordSignIn(TcpClient& client) {
    while (g_running) {
        system("clear");
        pushOpt("────── 密码登录 ──────\n");
        pushOpt("======================\n");
        pushOpt("请输入qq邮箱:\n");
        setPrefix("qq邮箱:");
        string email = popIpt();
        pushOpt("请输入密码:\n");
        setPrefix("密码:");
        string password = popIpt();
        pushOpt("======================\n");
        pushOpt("1. 登录    2. 返回\n");
        pushOpt("======================\n");
        pushOpt("请选择:\n");
        setPrefix("选择:");
        if (popIpt() == "1") {
            json req;
            req["type"] = 2;
            req["email"] = email;
            req["password"] = password;
            client.sendData(req.dump());
            json rsp = popRsp();
            if (rsp["code"] == 1) {
                home(client, rsp);
                return;
            } else {
                pushOpt("======================\n");
                pushOpt(string(rsp["msg"]) + "\n");
                pushOpt("1. 重新登录    2. 返回\n");
                pushOpt("======================\n");
                pushOpt("请选择:\n");
                setPrefix("选择:");
                if (popIpt() == "1") {
                    continue;
                } else {
                    return;
                }
            }
        } else {
            return;
        }
    }
}

void codeSignIn(TcpClient& client) {
    while (g_running) {
        system("clear");
        pushOpt("───── 验证码登录 ─────\n");
        pushOpt("======================\n");
        pushOpt("请输入qq邮箱:\n");
        setPrefix("qq邮箱:");
        string email = popIpt();
        pushOpt("正在发送验证码...");
        json req;
        req["type"] = 3;
        req["email"] = email;
        client.sendData(req.dump());
        json rsp = popRsp();
        if (rsp["code"] != 1) {
            pushOpt(string(rsp["msg"]) + "\n");
            pushOpt("======================\n");
            pushOpt("1. 重新输入    2. 返回\n");
            pushOpt("======================\n");
            pushOpt("请选择:\n");
            setPrefix("选择:");
            if (popIpt() != "1") {
                return;
            }
            continue;
        }
        pushOpt("验证码已发送，请输入验证码:\n");
        setPrefix("验证码:");
        string code = popIpt();
        pushOpt("======================\n");
        pushOpt("1. 登录    2. 返回\n");
        pushOpt("======================\n");
        pushOpt("请选择:\n");
        setPrefix("选择:");
        if (popIpt() == "1") {
            req["type"] = 4;
            req["code"] = code;
            client.sendData(req.dump());
            rsp = popRsp();
            if (rsp["code"] == 1) {
                home(client, rsp);
                return;
            } else {
                pushOpt("======================\n");
                pushOpt(string(rsp["msg"]) + "\n");
                pushOpt("1. 重新登录    2. 返回\n");
                pushOpt("======================\n");
                pushOpt("请选择:\n");
                setPrefix("选择:");
                if (popIpt() != "1") {
                    return;
                }
            }
        } else {
            return;
        }
    }
}

void home(TcpClient& client, const json& user) {
    while (g_running) {
        system("clear");
        pushOpt("────── " + string(user["name"]) + "，欢迎回来 ──────\n");
        pushOpt("======================\n");
        pushOpt("1. 好友\n");
        pushOpt("2. 群聊\n");
        pushOpt("3. 设置\n");
        pushOpt("4. 退出登录\n");
        pushOpt("======================\n");
        pushOpt("请选择:\n");
        setPrefix("选择:");
        string input = popIpt();
        if (!g_running || input.empty())
            continue;
        if (input == "1") {
            pushOpt("── 好友 ──\n");
        } else if (input == "2") {
            pushOpt("── 群聊 ──\n");
        } else if (input == "3") {
            if (!settings(client, user)) {
                return;
            }
            continue;
        } else if (input == "4") {
            return;
        }
    }
}

bool settings(TcpClient& client, const json& user) {
    while (g_running) {
        system("clear");
        pushOpt("────── 设置 ──────\n");
        pushOpt("======================\n");
        pushOpt("1. 更改密码\n");
        pushOpt("2. 注销账号\n");
        pushOpt("3. 退出登录\n");
        pushOpt("4. 返回\n");
        pushOpt("======================\n");
        pushOpt("请选择:\n");
        setPrefix("选择:");
        string input = popIpt();
        if (!g_running || input.empty())
            continue;
        if (input == "1") {
            system("clear");
            pushOpt("────── 更改密码 ──────\n");
            pushOpt("请输入新密码:\n");
            setPrefix("新密码:");
            string password = popIpt();
            pushOpt("正在发送验证码...\n");
            json req;
            req["type"] = 3;
            req["email"] = user["email"];
            client.sendData(req.dump());
            json rsp = popRsp();
            if (rsp["code"] != 1) {
                pushOpt(string(rsp["msg"]) + "\n");
                continue;
            }
            pushOpt("验证码已发送，请输入验证码:\n");
            setPrefix("验证码:");
            string code = popIpt();
            req["type"] = 5;
            req["code"] = code;
            req["password"] = password;
            client.sendData(req.dump());
            rsp = popRsp();
            pushOpt(string(rsp["msg"]) + "\n");
            continue;
        } else if (input == "2") {
            system("clear");
            pushOpt("────── 注销账号 ──────\n");
            pushOpt("确定要注销账号吗？此操作不可恢复！\n");
            pushOpt("1. 确认注销    2. 返回\n");
            pushOpt("请选择:\n");
            setPrefix("选择:");
            if (popIpt() == "1") {
                client.sendData(R"({"type":6})");
                json rsp = popRsp();
                pushOpt(string(rsp["msg"]) + "\n");
                return false;
            }
        } else if (input == "3") {
            return false;
        } else if (input == "4") {
            return true;
        }
    }
    return false;
}