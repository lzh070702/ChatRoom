#include "auth.h"

#include <cstdlib>

#include "friend.h"
#include "net/TcpClient.h"

static std::string formatOffline(const json& m) {
    int type = m["type"];
    if (type == 9) {
        return std::string(m["name"]) + std::string(m["msg"]);
    }
    if (type == 13) {
        bool is_file = m["msg_type"];
        return is_file ? "好友" + std::string(m["name"]) + "发来一个文件"
                       : "好友" + std::string(m["name"]) + "发来一条消息";
    }
    if (type == 17) {
        return std::string(m["msg"]) + std::string(m["group_name"]);
    }
    if (type == 18) {
        return std::string(m["msg"]) + std::string(m["name"]);
    }
    if (type == 19) {
        return std::string(m["msg"]);
    }
    if (type == 25) {
        bool is_file = m["msg_type"];
        std::string sender = m["sender_name"];
        return is_file ? sender + " 在群聊" + std::string(m["group_name"]) +
                             "发来一个文件"
                       : sender + " 在群聊" + std::string(m["group_name"]) +
                             "发来一条消息";
    }
    return "";
}

static void showOfflinePage(const json& offline) {
    system("clear");
    pushOpt("──────── 登录成功 ────────");
    pushOpt("======================");
    if (offline.empty()) {
        pushOpt("（暂无离线消息）");
    } else {
        for (auto& m : offline) {
            pushOpt(CYAN + formatOffline(m) + RESET);
        }
    }
    pushOpt("======================");
    setPrompt("（输入任意内容返回）");
    setPrefix("返回:");
    popIpt();
}

void authOptions(TcpClient& client) {
    while (g_running) {
        system("clear");
        pushOpt("────── 登录页面 ──────");
        pushOpt("======================");
        pushOpt("1. 密码登录");
        pushOpt("2. 验证码登录");
        pushOpt("3. 注册");
        pushOpt("4. 退出");
        pushOpt("======================");
        setPrompt("请选择:");
        setPrefix("选择:");
        std::string input = popIpt();
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
        pushOpt("──────── 注册 ────────");
        pushOpt("======================");
        setPrompt("请输入qq邮箱:");
        setPrefix("qq邮箱:");
        std::string email = popIpt();
        setPrompt("请输入用户名:");
        setPrefix("用户名:");
        std::string name = popIpt();
        setPrompt("请输入密码:");
        setPrefix("密码:");
        std::string password = popIpt();
        if (!confirm("确定注册吗")) {
            return;
        }
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
            showTip(GREEN + std::string(rsp["msg"]) + RESET);
            return;
        }
        pushOpt("======================");
        pushOpt(RED + std::string(rsp["msg"]) + RESET);
        pushOpt("1. 重新注册    2. 返回");
        pushOpt("======================");
        setPrompt("请选择:");
        setPrefix("选择:");
        while (g_running) {
            std::string input = popIpt();
            if (input == "1") {
                break;
            } else if (input == "2") {
                return;
            }
            pushOpt("\033[A\033[K请选择:");
        }
    }
}

void passwordSignIn(TcpClient& client) {
    while (g_running) {
        system("clear");
        pushOpt("────── 密码登录 ──────");
        pushOpt("======================");
        setPrompt("请输入qq邮箱:");
        setPrefix("qq邮箱:");
        std::string email = popIpt();
        setPrompt("请输入密码:");
        setPrefix("密码:");
        std::string password = popIpt();
        if (!confirm("确定登录吗")) {
            return;
        }
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
            json offline = rsp.value("offline", json::array());
            g_user = rsp;
            g_user.erase("offline");
            showOfflinePage(offline);
            home(client);
            return;
        }
        pushOpt("======================");
        pushOpt(RED + std::string(rsp["msg"]) + RESET);
        pushOpt("1. 重新登录    2. 返回");
        pushOpt("======================");
        setPrompt("请选择:");
        setPrefix("选择:");
        while (g_running) {
            std::string input = popIpt();
            if (input == "1") {
                break;
            } else if (input == "2") {
                return;
            }
            pushOpt("\033[A\033[K请选择:");
        }
    }
}

void codeSignIn(TcpClient& client) {
    while (g_running) {
        system("clear");
        pushOpt("───── 验证码登录 ─────");
        pushOpt("======================");
        setPrompt("请输入qq邮箱:");
        setPrefix("qq邮箱:");
        std::string email = popIpt();
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
            pushOpt("======================");
            pushOpt(RED + std::string(rsp["msg"]) + RESET);
            pushOpt("1. 重新输入    2. 返回");
            pushOpt("======================");
            setPrompt("请选择:");
            setPrefix("选择:");
            while (g_running) {
                std::string input = popIpt();
                if (input == "1") {
                    break;
                } else if (input == "2") {
                    return;
                }
                pushOpt("\033[A\033[K请选择:");
            }
            continue;
        }
        setPrompt("验证码已发送，请输入验证码:");
        setPrefix("验证码:");
        std::string code = popIpt();
        if (!confirm("确定登录吗")) {
            return;
        }
        req["type"] = 4;
        req["code"] = code;
        client.sendData(req.dump());
        rsp = popRsp();
        if (!g_running) {
            return;
        }
        if (rsp["code"] == 1) {
            json offline = rsp.value("offline", json::array());
            g_user = rsp;
            g_user.erase("offline");
            showOfflinePage(offline);
            home(client);
            return;
        }
        pushOpt("======================");
        pushOpt(RED + std::string(rsp["msg"]) + RESET);
        pushOpt("1. 重新登录    2. 返回");
        pushOpt("======================");
        setPrompt("请选择:");
        setPrefix("选择:");
        while (g_running) {
            std::string input = popIpt();
            if (input == "1") {
                break;
            } else if (input == "2") {
                return;
            }
            pushOpt("\033[A\033[K请选择:");
        }
    }
}