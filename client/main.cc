#include <readline/readline.h>
#include <signal.h>
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
queue<string> g_ipt_que;
condition_variable g_ipt_cv;

mutex g_rsp_mtx;
queue<json> g_rsp_que;
condition_variable g_rsp_cv;

mutex g_ui_mutex;
string g_prefix;
json g_user;

mutex g_chat_mtx;
int g_chat_type = -1;  // -1 无会话, 0 私聊, 1 群聊
int g_chat_id = -1;    // 私聊为对方好友 id, 群聊为群 id

void netLoop(TcpClient* client);
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
void showTip(const string& msg);
bool confirm(const string& msg);
void setChatSession(int type, int id);
bool isCurrentSession(int type, int id);

void authOptions(TcpClient& client);
void signUp(TcpClient& client);
void passwordSignIn(TcpClient& client);
void codeSignIn(TcpClient& client);
void home(TcpClient& client);
void friendPage(TcpClient& client);
void addFriend(TcpClient& client);
void friendList(TcpClient& client);
void friendMenu(TcpClient& client, const json& f);
void chat(TcpClient& client, const json& f);
bool settings(TcpClient& client);
void changePassword(TcpClient& client);
bool signOut(TcpClient& client);
bool exitLogin(TcpClient& client);

int main(int argc, char* argv[]) {
    signal(SIGPIPE, SIG_IGN);
    string host = (argc > 1) ? argv[1] : "127.0.0.1";
    int port = (argc > 2) ? stoi(argv[2]) : 8000;
    TcpClient client;
    if (!client.connectServer(host, port)) {
        cerr << "连接失败" << endl;
        return 1;
    }
    g_opt_efd = eventfd(0, EFD_NONBLOCK);

    pool pool(4);
    pool.enqueue(netLoop, &client);
    pool.enqueue(ioLoop);
    authOptions(client);
    return 0;
}

void netLoop(TcpClient* client) {
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
        client->flush();
        for (int i = 0; i < n; ++i) {
            if (events[i].data.fd != client->getFd()) {
                continue;
            }
            string msg = client->recvData();
            while (!msg.empty()) {
                parseOpt(msg, last_recv);
                msg = client->recvData();
            }
            if (client->isClosed()) {
                pushOpt("与服务器断开连接\n");
                g_running = false;
                g_ipt_cv.notify_all();
                g_rsp_cv.notify_all();
                break;
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
            if (errno == EINTR) {
                continue;
            }
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
        if (type == 9) {
            rsp = string(js["name"]) + string(js["msg"]);
        }
        if (type == 13) {
            if (isCurrentSession(0, js["id"])) {
                rsp = string(js["msg"]);
            } else {
                rsp = "收到一条消息";
            }
        }
        if (type == 17) {
            rsp = string(js["msg"]) + string(js["group_name"]);
        }
        if (type == 18) {
            rsp = string(js["msg"]) + string(js["name"]);
        }
        if (type == 19) {
            rsp = string(js["msg"]);
        }
        if (type == 25) {
            if (isCurrentSession(1, js["group_id"])) {
                rsp = string(js["msg"]);
            } else {
                rsp = "收到一条消息";
            }
        }
        pushOpt(rsp + "\n");
        return;
    }
    pushRsp(js);
}

void printOpt() {
    auto msgs = popOpt();
    if (msgs.empty()) {
        return;
    }

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
        g_ipt_que.push(smsg);
    }
    g_ipt_cv.notify_one();
}

string popIpt() {
    unique_lock<mutex> lk(g_ipt_mtx);
    g_ipt_cv.wait(lk, [] { return !g_ipt_que.empty() || !g_running; });
    if (!g_running || g_ipt_que.empty()) {
        return "";
    }
    string s = g_ipt_que.front();
    g_ipt_que.pop();
    return s;
}

void pushRsp(const json& js) {
    {
        lock_guard<mutex> lk(g_rsp_mtx);
        g_rsp_que.push(js);
    }
    g_rsp_cv.notify_one();
}

json popRsp() {
    unique_lock<mutex> lk(g_rsp_mtx);
    g_rsp_cv.wait(lk, [] { return !g_rsp_que.empty() || !g_running; });
    if (!g_running || g_rsp_que.empty()) {
        return json();
    }
    json js = g_rsp_que.front();
    g_rsp_que.pop();
    return js;
}

void setPrefix(const string& prefix) {
    lock_guard<mutex> lk(g_ui_mutex);
    g_prefix = prefix;
}

void showTip(const string& msg) {
    system("clear");
    pushOpt(msg + "\n");
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

bool confirm(const string& msg) {
    system("clear");
    pushOpt("======================\n");
    pushOpt(msg + "\n");
    pushOpt("1. 确认    2. 取消\n");
    pushOpt("======================\n");
    pushOpt("请选择:\n");
    setPrefix("选择:");
    while (g_running) {
        string input = popIpt();
        if (input == "1") {
            return true;
        } else if (input == "2") {
            return false;
        }
        pushOpt("\033[A\033[K请选择:\n");
    }
    return false;
}

void setChatSession(int type, int id) {
    lock_guard<mutex> lk(g_chat_mtx);
    g_chat_type = type;
    g_chat_id = id;
}

bool isCurrentSession(int type, int id) {
    lock_guard<mutex> lk(g_chat_mtx);
    return g_chat_type == type && g_chat_id == id;
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
        bool ctn = false;
        while (g_running) {
            string input = popIpt();
            if (input == "1") {
                json req;
                req["type"] = 1;
                req["email"] = email;
                req["name"] = name;
                req["password"] = password;
                client.sendData(req.dump());
                json rsp = popRsp();
                if (!g_running) {
                    return;
                }
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
                    while (g_running) {
                        string input = popIpt();
                        if (input == "1") {
                            ctn = true;
                            break;
                        } else if (input == "2") {
                            return;
                        }
                        pushOpt("\033[A\033[K请选择:\n");
                    }
                }
            } else if (input == "2") {
                return;
            }
            if (ctn) {
                break;
            }
            pushOpt("\033[A\033[K请选择:\n");
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
        bool ctn = false;
        while (g_running) {
            string input = popIpt();
            if (input == "1") {
                json req;
                req["type"] = 2;
                req["email"] = email;
                req["password"] = password;
                client.sendData(req.dump());
                json rsp = popRsp();
                if (!g_running) {
                    return;
                }
                if (rsp["code"] == 1) {
                    g_user = rsp;
                    home(client);
                    return;
                } else {
                    pushOpt("======================\n");
                    pushOpt(string(rsp["msg"]) + "\n");
                    pushOpt("1. 重新登录    2. 返回\n");
                    pushOpt("======================\n");
                    pushOpt("请选择:\n");
                    setPrefix("选择:");
                    while (g_running) {
                        string input = popIpt();
                        if (input == "1") {
                            ctn = true;
                            break;
                        } else if (input == "2") {
                            return;
                        }
                        pushOpt("\033[A\033[K请选择:\n");
                    }
                }
            } else if (input == "2") {
                return;
            }
            if (ctn) {
                break;
            }
            pushOpt("\033[A\033[K请选择:\n");
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
        if (!g_running) {
            return;
        }
        if (rsp["code"] != 1) {
            pushOpt("======================\n");
            pushOpt(string(rsp["msg"]) + "\n");
            pushOpt("1. 重新输入    2. 返回\n");
            pushOpt("======================\n");
            pushOpt("请选择:\n");
            setPrefix("选择:");
            while (g_running) {
                string input = popIpt();
                if (input == "1") {
                    break;
                } else if (input == "2") {
                    return;
                }
                pushOpt("\033[A\033[K请选择:\n");
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
        bool ctn = false;
        while (g_running) {
            string input = popIpt();
            if (input == "1") {
                req["type"] = 4;
                req["code"] = code;
                client.sendData(req.dump());
                rsp = popRsp();
                if (!g_running) {
                    return;
                }
                if (rsp["code"] == 1) {
                    g_user = rsp;
                    home(client);
                    return;
                } else {
                    pushOpt("======================\n");
                    pushOpt(string(rsp["msg"]) + "\n");
                    pushOpt("1. 重新登录    2. 返回\n");
                    pushOpt("======================\n");
                    pushOpt("请选择:\n");
                    setPrefix("选择:");
                    while (g_running) {
                        string input = popIpt();
                        if (input == "1") {
                            ctn = true;
                            break;
                        } else if (input == "2") {
                            return;
                        }
                        pushOpt("\033[A\033[K请选择:\n");
                    }
                }
            } else if (input == "2") {
                return;
            }
            if (ctn) {
                break;
            }
            pushOpt("\033[A\033[K请选择:\n");
        }
    }
}

void home(TcpClient& client) {
    while (g_running) {
        system("clear");
        pushOpt("──────── 首页 ────────\n");
        pushOpt("======================\n");
        pushOpt("1. 好友\n");
        pushOpt("2. 群聊\n");
        pushOpt("3. 设置\n");
        pushOpt("======================\n");
        pushOpt("请选择:\n");
        setPrefix("选择:");
        string input = popIpt();
        if (input == "1") {
            friendPage(client);
        } else if (input == "2") {
            pushOpt("── 群聊 ──\n");
        } else if (input == "3") {
            if (!settings(client)) {
                return;
            }
        }
    }
}

void friendPage(TcpClient& client) {
    while (g_running) {
        system("clear");
        pushOpt("──────── 好友 ────────\n");
        pushOpt("======================\n");
        pushOpt("1. 添加好友\n");
        pushOpt("2. 好友列表\n");
        pushOpt("3. 返回\n");
        pushOpt("======================\n");
        pushOpt("请选择:\n");
        setPrefix("选择:");
        string input = popIpt();
        if (input == "1") {
            addFriend(client);
        } else if (input == "2") {
            friendList(client);
        } else if (input == "3") {
            return;
        }
    }
}

void addFriend(TcpClient& client) {
    while (g_running) {
        system("clear");
        pushOpt("────── 添加好友 ──────\n");
        pushOpt("======================\n");
        pushOpt("请输入对方邮箱:\n");
        setPrefix("对方邮箱:");
        string email = popIpt();
        json req;
        req["type"] = 9;
        req["email"] = email;
        req["my_email"] = g_user["email"];
        req["name"] = g_user["name"];
        client.sendData(req.dump());
        json rsp = popRsp();
        if (!g_running) {
            return;
        }
        pushOpt(string(rsp["msg"]) + "\n");
        pushOpt("======================\n");
        pushOpt("1. 继续添加    2. 返回\n");
        pushOpt("======================\n");
        pushOpt("请选择:\n");
        setPrefix("选择:");
        while (g_running) {
            string input = popIpt();
            if (input == "1") {
                break;
            } else if (input == "2") {
                return;
            }
            pushOpt("\033[A\033[K请选择:\n");
        }
    }
}

void friendList(TcpClient& client) {
    while (g_running) {
        system("clear");
        json req;
        req["type"] = 8;
        client.sendData(req.dump());
        json rsp = popRsp();
        if (!g_running) {
            return;
        }
        if (rsp["code"] != 1) {
            showTip(string(rsp["msg"]));
            return;
        }
        pushOpt("────── 好友列表 ──────\n");
        pushOpt("======================\n");
        pushOpt("0. 返回\n");
        int cnt = 0;
        for (auto& f : rsp["friends"]) {
            cnt++;
            int id = f["id"];
            int state = f["state"];
            int status = f["status"];
            string relation;
            if (status == 2) {
                relation = "好友";
            } else if (status == 3) {
                relation = "已拉黑";
            } else if (status == 0) {
                relation = "待我处理";
            } else {
                relation = "待对方同意";
            }
            string online = (state == 1) ? "在线" : "离线";
            pushOpt(to_string(id) + " " + string(f["email"]) + " " + relation +
                    " " + online + "\n");
        }
        if (cnt == 0) {
            pushOpt("（暂无好友）\n");
        }
        pushOpt("======================\n");
        pushOpt("请输入 id 或 0 :\n");
        setPrefix("好友id:");
        string input = popIpt();
        if (input == "0") {
            return;
        }
        for (auto& f : rsp["friends"]) {
            int fid = f["id"];
            if (to_string(fid) == input) {
                friendMenu(client, f);
                break;
            }
        }
    }
}

void friendMenu(TcpClient& client, const json& f) {
    int id = f["id"];
    int status = f["status"];
    while (g_running) {
        system("clear");
        pushOpt("────── 好友操作 ──────\n");
        pushOpt("======================\n");
        pushOpt(string(f["name"]) + "(" + string(f["email"]) + ")\n");
        pushOpt("======================\n");
        if (status == 2) {
            pushOpt("1. 私聊\n");
            pushOpt("2. 查看聊天记录\n");
            pushOpt("3. 拉黑\n");
            pushOpt("4. 删除\n");
            pushOpt("5. 返回\n");
        } else if (status == 3) {
            pushOpt("1. 查看聊天记录\n");
            pushOpt("2. 取消拉黑\n");
            pushOpt("3. 删除\n");
            pushOpt("4. 返回\n");
        } else if (status == 0) {
            pushOpt("1. 同意\n");
            pushOpt("2. 拒绝\n");
            pushOpt("3. 返回\n");
        } else {
            pushOpt("等待对方同意\n");
            pushOpt("1. 返回\n");
        }
        pushOpt("======================\n");
        pushOpt("请选择:\n");
        setPrefix("选择:");
        string input = popIpt();
        json req;
        bool send = false;
        if (status == 2) {
            if (input == "1") {
                chat(client, f);
                continue;
            } else if (input == "2") {
                showTip("查看聊天记录功能待实现");
                continue;
            } else if (input == "3") {
                req["type"] = 11;
                req["id"] = id;
                req["block"] = true;
                send = true;
            } else if (input == "4") {
                if (!confirm("确定要删除好友 " + string(f["name"]) + " 吗？")) {
                    continue;
                }
                req["type"] = 12;
                req["id"] = id;
                send = true;
            } else if (input == "5") {
                return;
            }
        } else if (status == 3) {
            if (input == "1") {
                showTip("查看聊天记录功能待实现");
                continue;
            } else if (input == "2") {
                req["type"] = 11;
                req["id"] = id;
                req["block"] = false;
                send = true;
            } else if (input == "3") {
                if (!confirm("确定要删除好友 " + string(f["name"]) + " 吗？")) {
                    continue;
                }
                req["type"] = 12;
                req["id"] = id;
                send = true;
            } else if (input == "4") {
                return;
            }
        } else if (status == 0) {
            if (input == "1") {
                req["type"] = 10;
                req["id"] = id;
                req["agree"] = 1;
                send = true;
            } else if (input == "2") {
                req["type"] = 10;
                req["id"] = id;
                req["agree"] = 0;
                send = true;
            } else if (input == "3") {
                return;
            }
        } else {
            if (input == "1") {
                return;
            }
        }
        if (!send) {
            continue;
        }

        client.sendData(req.dump());
        json rsp = popRsp();
        if (!g_running) {
            return;
        }
        showTip(string(rsp["msg"]));
        return;
    }
}

void chat(TcpClient& client, const json& f) {
    int id = f["id"];
    string name = f["name"];
    setChatSession(0, id);
    system("clear");
    pushOpt("────── 与 " + name + " 私聊 ──────\n");
    pushOpt("======================\n");
    pushOpt("（输入消息，/q 退出）\n");
    pushOpt("======================\n");
    setPrefix("我:");
    while (g_running) {
        string input = popIpt();
        if (!g_running) {
            break;
        }
        if (input == "/q") {
            break;
        }
        json req;
        req["type"] = 13;
        req["id"] = id;
        req["msg"] = input;
        req["msg_type"] = false;
        client.sendData(req.dump());
    }
    setChatSession(-1, -1);
}

bool settings(TcpClient& client) {
    while (g_running) {
        system("clear");
        pushOpt("──────── 设置 ────────\n");
        pushOpt("======================\n");
        pushOpt("1. 更改密码\n");
        pushOpt("2. 注销账号\n");
        pushOpt("3. 退出登录\n");
        pushOpt("4. 返回\n");
        pushOpt("======================\n");
        pushOpt("请选择:\n");
        setPrefix("选择:");
        string input = popIpt();
        if (input == "1") {
            changePassword(client);
            continue;
        } else if (input == "2") {
            if (signOut(client)) {
                return false;
            }
            continue;
        } else if (input == "3") {
            if (exitLogin(client)) {
                return false;
            }
        } else if (input == "4") {
            return true;
        }
    }
    return false;
}

void changePassword(TcpClient& client) {  //
    system("clear");
    pushOpt("────── 更改密码 ──────\n");
    pushOpt("======================\n");
    pushOpt("请输入新密码:\n");
    setPrefix("新密码:");
    string password = popIpt();
    json req;
    req["type"] = 3;
    req["email"] = g_user["email"];
    while (g_running) {
        pushOpt("正在发送验证码...");
        client.sendData(req.dump());
        json rsp = popRsp();
        if (!g_running) {
            return;
        }
        if (rsp["code"] != 1) {
            pushOpt("======================\n");
            pushOpt(string(rsp["msg"]) + "\n");
            pushOpt("是否重新发送？\n");
            pushOpt("1. 确认    2. 返回\n");
            pushOpt("======================\n");
            pushOpt("请选择\n");
            setPrefix("选择:");
            while (g_running) {
                string input = popIpt();
                if (input == "1") {
                    pushOpt("\033[A\033[K\033[A\033[K\033[A\033[K");
                    pushOpt("\033[A\033[K\033[A\033[K\033[A\033[K");
                    break;
                } else if (input == "2") {
                    return;
                }
                pushOpt("\033[A\033[K请选择:\n");
            }
        } else {
            break;
        }
    }
    while (g_running) {
        pushOpt("验证码已发送，请输入验证码:\n");
        setPrefix("验证码:");
        string code = popIpt();
        req["type"] = 5;
        req["code"] = code;
        req["password"] = password;
        client.sendData(req.dump());
        json rsp = popRsp();
        if (!g_running) {
            return;
        }
        if (rsp["code"] != 1) {
            pushOpt("======================\n");
            pushOpt(string(rsp["msg"]) + "\n");
            pushOpt("是否重新输入？\n");
            pushOpt("1. 确认    2. 返回\n");
            pushOpt("======================\n");
            pushOpt("请选择:\n");
            setPrefix("选择:");
            while (g_running) {
                string input = popIpt();
                if (input == "1") {
                    pushOpt("\033[A\033[K\033[A\033[K\033[A\033[K");
                    pushOpt("\033[A\033[K\033[A\033[K\033[A\033[K\033[A\033[K");
                    break;
                } else if (input == "2") {
                    return;
                }
                pushOpt("\033[A\033[K请选择:\n");
            }
        } else {
            break;
        }
    }
}

bool signOut(TcpClient& client) {
    system("clear");
    pushOpt("────── 注销账号 ──────\n");
    pushOpt("======================\n");
    pushOpt("确定要注销账号吗？此操作不可恢复！\n");
    pushOpt("1. 确认    2. 返回\n");
    pushOpt("======================\n");
    pushOpt("请选择:\n");
    setPrefix("选择:");
    while (g_running) {
        string input = popIpt();
        if (input == "1") {
            break;
        } else if (input == "2") {
            return false;
        }
        pushOpt("\033[A\033[K请选择:\n");
    }
    client.sendData(R"({"type":7})");
    json rsp = popRsp();
    if (!g_running) {
        return false;
    }
    pushOpt(string(rsp["msg"]) + "\n");
    g_user = json();
    return true;
}

bool exitLogin(TcpClient& client) {
    system("clear");
    pushOpt("────── 退出登录 ──────\n");
    pushOpt("======================\n");
    pushOpt("确定要退出登录吗？\n");
    pushOpt("1. 确认    2. 返回\n");
    pushOpt("======================\n");
    pushOpt("请选择:\n");
    setPrefix("选择:");
    while (g_running) {
        string input = popIpt();
        if (input == "1") {
            break;
        } else if (input == "2") {
            return false;
        }
        pushOpt("\033[A\033[K请选择:\n");
    }
    client.sendData(R"({"type":6})");
    json rsp = popRsp();
    if (!g_running) {
        return false;
    }
    pushOpt(string(rsp["msg"]) + "\n");
    g_user = json();
    return true;
}