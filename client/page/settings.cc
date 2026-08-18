#include "settings.h"

#include <cstdlib>

#include "net/TcpClient.h"

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
        std::string input = popIpt();
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
    std::string password = popIpt();
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
            pushOpt(std::string(rsp["msg"]) + "\n");
            pushOpt("是否重新发送？\n");
            pushOpt("1. 确认    2. 返回\n");
            pushOpt("======================\n");
            pushOpt("请选择\n");
            setPrefix("选择:");
            while (g_running) {
                std::string input = popIpt();
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
        std::string code = popIpt();
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
            pushOpt(std::string(rsp["msg"]) + "\n");
            pushOpt("是否重新输入？\n");
            pushOpt("1. 确认    2. 返回\n");
            pushOpt("======================\n");
            pushOpt("请选择:\n");
            setPrefix("选择:");
            while (g_running) {
                std::string input = popIpt();
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
        std::string input = popIpt();
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
    pushOpt(std::string(rsp["msg"]) + "\n");
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
        std::string input = popIpt();
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
    pushOpt(std::string(rsp["msg"]) + "\n");
    g_user = json();
    return true;
}