#include "friend.h"

#include <cstdlib>
#include <fstream>
#include <iterator>

#include "group.h"
#include "net/TcpClient.h"
#include "settings.h"

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
        std::string input = popIpt();
        if (input == "1") {
            friendPage(client);
        } else if (input == "2") {
            groupPage(client);
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
        std::string input = popIpt();
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
        std::string email = popIpt();
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
        pushOpt(std::string(rsp["msg"]) + "\n");
        pushOpt("======================\n");
        pushOpt("1. 继续添加    2. 返回\n");
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
            showTip(std::string(rsp["msg"]));
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
            std::string relation;
            if (status == 2) {
                relation = "好友";
            } else if (status == 3) {
                relation = "已拉黑";
            } else if (status == 0) {
                relation = "待我处理";
            } else {
                relation = "待对方同意";
            }
            std::string online = (state == 1) ? "在线" : "离线";
            pushOpt(std::to_string(id) + " " + std::string(f["email"]) + " " +
                    relation + " " + online + "\n");
        }
        if (cnt == 0) {
            pushOpt("（暂无好友）\n");
        }
        pushOpt("======================\n");
        pushOpt("请输入 id 或 0 :\n");
        setPrefix("好友id:");
        std::string input = popIpt();
        if (input == "0") {
            return;
        }
        for (auto& f : rsp["friends"]) {
            int fid = f["id"];
            if (std::to_string(fid) == input) {
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
        pushOpt(std::string(f["name"]) + "(" + std::string(f["email"]) + ")\n");
        pushOpt("======================\n");
        if (status == 2) {
            pushOpt("1. 私聊\n");
            pushOpt("2. 查看聊天记录\n");
            pushOpt("3. 拉黑\n");
            pushOpt("4. 删除\n");
            pushOpt("5. 返回\n");
        } else if (status == 3) {
            pushOpt("1. 私聊\n");
            pushOpt("2. 查看聊天记录\n");
            pushOpt("3. 取消拉黑\n");
            pushOpt("4. 删除\n");
            pushOpt("5. 返回\n");
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
        std::string input = popIpt();
        json req;
        bool send = false;
        if (status == 2) {
            if (input == "1") {
                chat(client, f);
                continue;
            } else if (input == "2") {
                viewHistory(client, f);
                continue;
            } else if (input == "3") {
                req["type"] = 11;
                req["id"] = id;
                req["block"] = true;
                send = true;
            } else if (input == "4") {
                if (!confirm("确定要删除好友 " + std::string(f["name"]) +
                             " 吗？")) {
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
                chat(client, f);
                continue;
            } else if (input == "2") {
                viewHistory(client, f);
                continue;
            } else if (input == "3") {
                req["type"] = 11;
                req["id"] = id;
                req["block"] = false;
                send = true;
            } else if (input == "4") {
                if (!confirm("确定要删除好友 " + std::string(f["name"]) +
                             " 吗？")) {
                    continue;
                }
                req["type"] = 12;
                req["id"] = id;
                send = true;
            } else if (input == "5") {
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
        showTip(std::string(rsp["msg"]));
        return;
    }
}

void chat(TcpClient& client, const json& f) {
    int id = f["id"];
    std::string name = f["name"];
    setChatSession(0, id);
    system("clear");
    pushOpt("私聊: " + name + "\n");
    pushOpt("（输入消息，/q 退出）\n");
    pushOpt("======================\n");
    showHistory(client, f, 0);
    pushOpt("======================\n");
    setPrefix("我:");
    while (g_running) {
        std::string input = popIpt();
        if (!g_running) {
            break;
        }
        if (input == "/q") {
            break;
        }
        if (input.rfind("put ", 0) == 0) {
            uploadFile(client, id, input.substr(4));
            continue;
        }
        if (input.rfind("get ", 0) == 0) {
            downloadFile(client, input.substr(4));
            continue;
        }
        json req;
        req["type"] = 13;
        req["id"] = id;
        req["msg"] = input;
        req["msg_type"] = false;
        client.sendData(req.dump());
        json rsp = popRsp();
        if (rsp["code"] != 1) {
            pushOpt(std::string(rsp["msg"]) + "\n");
        }
        pushOpt("======================\n");
    }
    setChatSession(-1, -1);
}

void showHistory(TcpClient& client, const json& f, int scope) {
    int id = f["id"];
    std::string name = f["name"];
    int my_id = g_user["id"];
    json req;
    req["type"] = 14;
    req["id"] = id;
    req["scope"] = scope;
    client.sendData(req.dump());
    json rsp = popRsp();
    if (!g_running) {
        return;
    }
    if (rsp["code"] != 1) {
        showTip(std::string(rsp["msg"]));
        return;
    }
    if (rsp["msg"].empty()) {
        pushOpt("（暂无聊天记录）\n");
    }
    for (auto& m : rsp["msg"]) {
        int sender_id = m["sender_id"];
        int is_file = m["is_file"];
        std::string content = m["content"];
        if (is_file == 1) {
            content = "[文件] " + content;
        }
        std::string who = (sender_id == my_id) ? "我" : name;
        pushOpt(who + ": " + content + "\n");
    }
}

void viewHistory(TcpClient& client, const json& f) {
    system("clear");
    pushOpt("======================\n");
    showHistory(client, f, 1);
    pushOpt("======================\n");
    pushOpt("聊天记录: " + std::string(f["name"]) + "\n");
    pushOpt("（输入任意内容返回）\n");
    setPrefix("返回:");
    popIpt();
}

void uploadFile(TcpClient& client, int id, const std::string& path) {
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) {
        pushOpt("文件不存在: " + path + "\n");
        return;
    }
    std::string data((std::istreambuf_iterator<char>(ifs)),
                     std::istreambuf_iterator<char>());
    std::string name = path.substr(path.find_last_of('/') + 1);
    json req;
    req["type"] = 13;
    req["id"] = id;
    req["msg"] = name;
    req["msg_type"] = true;
    req["file_data"] = base64Encode(data);
    client.sendData(req.dump());
    json rsp = popRsp();
    if (rsp["code"] != 1) {
        pushOpt(std::string(rsp["msg"]) + "\n");
    } else {
        pushOpt("文件已发送\n");
    }
    pushOpt("======================\n");
}

void downloadFile(TcpClient& client, const std::string& ref) {
    json req;
    req["type"] = 27;
    req["msg"] = ref;
    client.sendData(req.dump());
    json rsp = popRsp();
    if (rsp["code"] != 1) {
        pushOpt(std::string(rsp["msg"]) + "\n");
        return;
    }
    std::string name = ref;
    size_t pos = name.find('_');
    if (pos != std::string::npos) {
        name = name.substr(pos + 1);
    }
    std::string data = base64Decode(std::string(rsp["file_data"]));
    std::ofstream ofs("./downloads/" + name, std::ios::binary);
    ofs.write(data.data(), data.size());
    pushOpt("文件已保存为: downloads/" + name + "\n");
    pushOpt("======================\n");
}