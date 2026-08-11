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
void parseOpt(const string& msg);
void printOpt();
void pushOpt(const string& smsg);
vector<string> popOpt();
void pushIpt(const string& smsg);
string popIpt();
void pushRsp(const json& js);
json popRsp();
void setPrefix(const string& prefix);

void home();
void codelogin();

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
    pool.enqueue(home);
    return 0;
}

void netLoop(TcpClient* client, pool* pool) {
    int epfd = epoll_create1(0);
    epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.fd = client->getFd();
    epoll_ctl(epfd, EPOLL_CTL_ADD, client->getFd(), &ev);

    epoll_event events[4];
    while (g_running) {
        int n = epoll_wait(epfd, events, 4, 500);
        for (int i = 0; i < n; ++i) {
            if (events[i].data.fd != client->getFd()) {
                continue;
            }
            string msg = client->recvData();
            while (!msg.empty()) {
                pool->enqueue(parseOpt, msg);
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
        return;
    }
    string prefix;
    {
        lock_guard<mutex> lk(g_ui_mutex);
        prefix = g_prefix;
    }
    if (*line) {
        fprintf(rl_outstream, "\033[A\033[K%s %s\n", prefix.c_str(), line);
        pushIpt(line);
    } else {
        fprintf(rl_outstream, "\033[A");
    }
    free(line);
    rl_callback_handler_remove();
    rl_callback_handler_install("", on_line);
}

void parseOpt(const string& msg) {
    json js;
    try {
        js = json::parse(msg);
    } catch (...) {
        return;
    }
    int code = js["code"];
    int type = js["type"];
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

void home() {
    while (g_running) {
        system("clear");
        pushOpt("── 登录页面 ──\n");
        pushOpt("1. 密码登录\n");
        pushOpt("2. 验证码登录\n");
        pushOpt("3. 注册\n");
        pushOpt("请输入 1、2、3 或 /quit：");
        setPrefix("选择：");
        string input = popIpt();
        if (!g_running || input.empty())
            continue;

        if (input == "/quit") {
            g_running = false;
            return;
        }
        if (input == "1") {
            codelogin();
        } else if (input == "2") {
            pushOpt("── 验证码登录 ──\n");
        } else if (input == "3") {
            pushOpt("── 注册 ──\n");
        }
    }
}

void codelogin() {
    while (g_running) {
        system("clear");
        pushOpt("── 密码登录 ──\n");
        pushOpt("请输入qq邮箱：");
        setPrefix("qq邮箱：");
        string email = popIpt();
        if(email == "q"){
            return;
        }
        pushOpt("请输入密码：");
        setPrefix("密码：");
        string password = popIpt();
        pushOpt("按下回车登录");
        string a ;
        cin >> a;
    }
}