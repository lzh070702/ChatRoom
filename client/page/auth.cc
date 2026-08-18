#include "auth.h"

#include <cstdlib>

#include "friend.h"
#include "net/TcpClient.h"

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
        pushOpt("──────── 注册 ────────\n");
        pushOpt("======================\n");
        pushOpt("请输入qq邮箱:\n");
        setPrefix("qq邮箱:");
        std::string email = popIpt();
        pushOpt("请输入用户名:\n");
        setPrefix("用户名:");
        std::string name = popIpt();
        pushOpt("请输入密码:\n");
        setPrefix("密码:");
        std::string password = popIpt();
        pushOpt("======================\n");
        pushOpt("1. 注册    2. 返回\n");
        pushOpt("======================\n");
        pushOpt("请选择:\n");
        setPrefix("选择:");
        bool ctn = false;
        while (g_running) {
            std::string input = popIpt();
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
                    pushOpt(std::string(rsp["msg"]) + "\n");
                    return;
                } else {
                    pushOpt("======================\n");
                    pushOpt(std::string(rsp["msg"]) + "\n");
                    pushOpt("1. 重新注册    2. 返回\n");
                    pushOpt("======================\n");
                    pushOpt("请选择:\n");
                    setPrefix("选择:");
                    while (g_running) {
                        std::string input = popIpt();
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
        std::string email = popIpt();
        pushOpt("请输入密码:\n");
        setPrefix("密码:");
        std::string password = popIpt();
        pushOpt("======================\n");
        pushOpt("1. 登录    2. 返回\n");
        pushOpt("======================\n");
        pushOpt("请选择:\n");
        setPrefix("选择:");
        bool ctn = false;
        while (g_running) {
            std::string input = popIpt();
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
                    pushOpt(std::string(rsp["msg"]) + "\n");
                    pushOpt("1. 重新登录    2. 返回\n");
                    pushOpt("======================\n");
                    pushOpt("请选择:\n");
                    setPrefix("选择:");
                    while (g_running) {
                        std::string input = popIpt();
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
            pushOpt("======================\n");
            pushOpt(std::string(rsp["msg"]) + "\n");
            pushOpt("1. 重新输入    2. 返回\n");
            pushOpt("======================\n");
            pushOpt("请选择:\n");
            setPrefix("选择:");
            while (g_running) {
                std::string input = popIpt();
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
        std::string code = popIpt();
        pushOpt("======================\n");
        pushOpt("1. 登录    2. 返回\n");
        pushOpt("======================\n");
        pushOpt("请选择:\n");
        setPrefix("选择:");
        bool ctn = false;
        while (g_running) {
            std::string input = popIpt();
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
                    pushOpt(std::string(rsp["msg"]) + "\n");
                    pushOpt("1. 重新登录    2. 返回\n");
                    pushOpt("======================\n");
                    pushOpt("请选择:\n");
                    setPrefix("选择:");
                    while (g_running) {
                        std::string input = popIpt();
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