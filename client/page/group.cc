#include "group.h"

#include <cstdlib>
#include <fstream>
#include <iterator>

#include "friend.h"
#include "net/TcpClient.h"

void groupPage(TcpClient& client) {
    while (g_running) {
        system("clear");
        pushOpt("──────── 群聊 ────────\n");
        pushOpt("======================\n");
        pushOpt("1. 我的群聊\n");
        pushOpt("2. 创建群聊\n");
        pushOpt("3. 申请入群\n");
        pushOpt("4. 返回\n");
        pushOpt("======================\n");
        pushOpt("请选择:\n");
        setPrefix("选择:");
        std::string input = popIpt();
        if (input == "1") {
            groupList(client);
        } else if (input == "2") {
            createGroup(client);
        } else if (input == "3") {
            applyGroup(client);
        } else if (input == "4") {
            return;
        }
    }
}

void groupList(TcpClient& client) {
    while (g_running) {
        system("clear");
        json req;
        req["type"] = 15;
        client.sendData(req.dump());
        json rsp = popRsp();
        if (!g_running) {
            return;
        }
        if (rsp["code"] != 1) {
            showTip(std::string(rsp["msg"]));
            return;
        }
        pushOpt("────── 我的群聊 ──────\n");
        pushOpt("======================\n");
        pushOpt("0. 返回\n");
        int cnt = 0;
        for (auto& g : rsp["msg"]) {
            cnt++;
            pushOpt(std::to_string(cnt) + ". " + std::string(g["name"]) +
                    " (id:" + std::to_string((int)g["id"]) + ")\n");
        }
        if (cnt == 0) {
            pushOpt("（暂无群聊）\n");
        }
        pushOpt("======================\n");
        pushOpt("请输入序号或 0 :\n");
        setPrefix("群序号:");
        std::string input = popIpt();
        if (input == "0") {
            return;
        }
        int idx = atoi(input.c_str());
        if (idx >= 1 && idx <= cnt) {
            groupMenu(client, rsp["msg"][idx - 1]);
        }
    }
}

void createGroup(TcpClient& client) {
    while (g_running) {
        system("clear");
        pushOpt("────── 创建群聊 ──────\n");
        pushOpt("======================\n");
        pushOpt("请输入群名称:\n");
        setPrefix("群名称:");
        std::string name = popIpt();
        if (name.empty()) {
            continue;
        }
        pushOpt("======================\n");
        pushOpt("1. 创建    2. 返回\n");
        pushOpt("======================\n");
        pushOpt("请选择:\n");
        setPrefix("选择:");
        while (g_running) {
            std::string input = popIpt();
            if (input == "1") {
                json req;
                req["type"] = 16;
                req["name"] = name;
                client.sendData(req.dump());
                json rsp = popRsp();
                if (!g_running) {
                    return;
                }
                showTip(std::string(rsp["msg"]));
                return;
            } else if (input == "2") {
                return;
            }
            pushOpt("\033[A\033[K请选择:\n");
        }
    }
}

void applyGroup(TcpClient& client) {
    while (g_running) {
        system("clear");
        pushOpt("────── 申请入群 ──────\n");
        pushOpt("======================\n");
        pushOpt("请输入群 id:\n");
        setPrefix("群id:");
        std::string gid = popIpt();
        if (gid.empty()) {
            continue;
        }
        json req;
        req["type"] = 18;
        req["group_id"] = atoi(gid.c_str());
        req["email"] = g_user["email"];
        req["name"] = g_user["name"];
        client.sendData(req.dump());
        json rsp = popRsp();
        if (!g_running) {
            return;
        }
        showTip(std::string(rsp["msg"]));
        return;
    }
}

void groupMenu(TcpClient& client, const json& g) {
    int group_id = g["id"];
    std::string group_name = g["name"];
    int my_id = g_user["id"];
    while (g_running) {
        json req;
        req["type"] = 20;
        req["group_id"] = group_id;
        client.sendData(req.dump());
        json rsp = popRsp();
        if (!g_running) {
            return;
        }
        if (rsp["code"] != 1) {
            showTip(std::string(rsp["msg"]));
            return;
        }
        int my_role = -1;
        for (auto& m : rsp["members"]) {
            if (m["id"] == my_id) {
                my_role = m["role"];
            }
        }
        system("clear");
        pushOpt("────── 群操作 ──────\n");
        pushOpt("======================\n");
        pushOpt(group_name + " (id:" + std::to_string(group_id) + ")\n");
        pushOpt("======================\n");
        for (auto& m : rsp["members"]) {
            int role = m["role"];
            std::string role_name = (role == 3)   ? "群主"
                                    : (role == 2) ? "管理员"
                                    : (role == 1) ? "成员"
                                                  : "待审核";
            std::string online = (m["state"] == 1) ? "在线" : "离线";
            pushOpt(std::to_string((int)m["id"]) + " " +
                    std::string(m["name"]) + " " + role_name + " " + online +
                    "\n");
        }
        pushOpt("======================\n");
        pushOpt("1. 进入群聊\n");
        pushOpt("2. 查看聊天记录\n");
        if (my_role >= 2) {
            pushOpt("3. 邀请好友入群\n");
            pushOpt("4. 处理入群申请\n");
            pushOpt("5. 踢出成员\n");
        }
        if (my_role == 3) {
            pushOpt("6. 设置/撤销管理员\n");
            pushOpt("7. 解散群\n");
        }
        if (my_role != 3) {
            pushOpt("8. 退出群聊\n");
        }
        pushOpt("0. 返回\n");
        pushOpt("======================\n");
        pushOpt("请选择:\n");
        setPrefix("选择:");
        std::string input = popIpt();
        if (!g_running) {
            return;
        }
        if (input == "1") {
            groupChatPage(client, g);
            continue;
        } else if (input == "2") {
            viewGroupHistory(client, g);
            continue;
        } else if (input == "0") {
            return;
        }

        if (input == "3" && my_role >= 2) {
            json freq;
            freq["type"] = 8;
            client.sendData(freq.dump());
            json frsp = popRsp();
            if (!g_running) {
                return;
            }
            if (frsp["code"] != 1) {
                showTip(std::string(frsp["msg"]));
                continue;
            }
            std::vector<int> friend_ids;
            system("clear");
            pushOpt("──── 邀请好友入群 ────\n");
            pushOpt("======================\n");
            pushOpt("0. 返回\n");
            int cnt = 0;
            for (auto& f : frsp["friends"]) {
                if (f["status"] == 2) {
                    cnt++;
                    friend_ids.push_back(f["id"]);
                    pushOpt(std::to_string(cnt) + ". " +
                            std::string(f["name"]) + "\n");
                }
            }
            if (cnt == 0) {
                pushOpt("（暂无好友可邀请）\n");
            }
            pushOpt("======================\n");
            pushOpt("请输入序号或 0 :\n");
            setPrefix("好友序号:");
            std::string sel = popIpt();
            if (sel == "0") {
                continue;
            }
            int idx = atoi(sel.c_str());
            if (idx < 1 || idx > cnt) {
                continue;
            }
            req["type"] = 17;
            req["group_id"] = group_id;
            req["id"] = friend_ids[idx - 1];
            client.sendData(req.dump());
            json rsp2 = popRsp();
            if (!g_running) {
                return;
            }
            showTip(std::string(rsp2["msg"]));
            continue;
        }

        if (input == "4" && my_role >= 2) {
            std::vector<int> applicants;
            system("clear");
            pushOpt("──── 处理入群申请 ────\n");
            pushOpt("======================\n");
            pushOpt("0. 返回\n");
            int cnt = 0;
            for (auto& m : rsp["members"]) {
                if (m["role"] == 0) {
                    cnt++;
                    applicants.push_back(m["id"]);
                    pushOpt(std::to_string(cnt) + ". " +
                            std::string(m["name"]) + " (" +
                            std::string(m["email"]) + ")\n");
                }
            }
            if (cnt == 0) {
                pushOpt("（暂无入群申请）\n");
            }
            pushOpt("======================\n");
            pushOpt("请输入序号或 0 :\n");
            setPrefix("申请序号:");
            std::string sel = popIpt();
            if (sel == "0") {
                continue;
            }
            int idx = atoi(sel.c_str());
            if (idx < 1 || idx > cnt) {
                continue;
            }
            pushOpt("1. 同意    2. 拒绝\n");
            setPrefix("选择:");
            std::string agree = popIpt();
            if (agree != "1" && agree != "2") {
                continue;
            }
            req["type"] = 19;
            req["group_id"] = group_id;
            req["id"] = applicants[idx - 1];
            req["agree"] = (agree == "1") ? 1 : 0;
            client.sendData(req.dump());
            json rsp2 = popRsp();
            if (!g_running) {
                return;
            }
            showTip(std::string(rsp2["msg"]));
            continue;
        }

        if (input == "5" && my_role >= 2) {
            pushOpt("请输入要踢出的成员 id:\n");
            setPrefix("成员id:");
            std::string sel = popIpt();
            if (sel.empty()) {
                continue;
            }
            int target = atoi(sel.c_str());
            if (!confirm("确定要踢出该成员吗？")) {
                continue;
            }
            req["type"] = 22;
            req["group_id"] = group_id;
            req["id"] = target;
            client.sendData(req.dump());
            json rsp2 = popRsp();
            if (!g_running) {
                return;
            }
            showTip(std::string(rsp2["msg"]));
            continue;
        }

        if (input == "6" && my_role == 3) {
            pushOpt("请输入成员 id:\n");
            setPrefix("成员id:");
            std::string sel = popIpt();
            if (sel.empty()) {
                continue;
            }
            int target = atoi(sel.c_str());
            pushOpt("1. 设为管理员    2. 撤销管理员\n");
            setPrefix("选择:");
            std::string choice = popIpt();
            if (choice != "1" && choice != "2") {
                continue;
            }
            req["type"] = 21;
            req["group_id"] = group_id;
            req["id"] = target;
            req["admin"] = (choice == "1") ? 1 : 0;
            client.sendData(req.dump());
            json rsp2 = popRsp();
            if (!g_running) {
                return;
            }
            showTip(std::string(rsp2["msg"]));
            continue;
        }

        if (input == "7" && my_role == 3) {
            if (!confirm("确定要解散该群吗？此操作不可恢复！")) {
                continue;
            }
            req["type"] = 24;
            req["group_id"] = group_id;
            client.sendData(req.dump());
            json rsp2 = popRsp();
            if (!g_running) {
                return;
            }
            showTip(std::string(rsp2["msg"]));
            return;
        }

        if (input == "8" && my_role != 3) {
            if (!confirm("确定要退出该群吗？")) {
                continue;
            }
            req["type"] = 23;
            req["group_id"] = group_id;
            client.sendData(req.dump());
            json rsp2 = popRsp();
            if (!g_running) {
                return;
            }
            showTip(std::string(rsp2["msg"]));
            return;
        }
        pushOpt("\033[A\033[K请选择:\n");
    }
}

void uploadGroupFile(TcpClient& client, int group_id, const std::string& path) {
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) {
        pushOpt("文件不存在: " + path + "\n");
        return;
    }
    std::string data((std::istreambuf_iterator<char>(ifs)),
                     std::istreambuf_iterator<char>());
    std::string name = path.substr(path.find_last_of('/') + 1);
    json req;
    req["type"] = 25;
    req["group_id"] = group_id;
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

void groupChatPage(TcpClient& client, const json& g) {
    int group_id = g["id"];
    std::string group_name = g["name"];
    setChatSession(1, group_id);
    system("clear");
    pushOpt("群聊: " + group_name + "\n");
    pushOpt("（输入消息，/q 退出，put <路径> 发文件）\n");
    pushOpt("======================\n");
    showGroupHistory(client, g, 0);
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
            uploadGroupFile(client, group_id, input.substr(4));
            continue;
        }
        if (input.rfind("get ", 0) == 0) {
            downloadFile(client, input.substr(4));
            continue;
        }
        json req;
        req["type"] = 25;
        req["group_id"] = group_id;
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

void showGroupHistory(TcpClient& client, const json& g, int scope) {
    int group_id = g["id"];
    int my_id = g_user["id"];
    json req;
    req["type"] = 26;
    req["group_id"] = group_id;
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
        std::string sender_name = m["sender_name"];
        if (is_file == 1) {
            content = "[文件] " + content;
        }
        std::string who = (sender_id == my_id) ? "我" : sender_name;
        pushOpt(who + ": " + content + "\n");
    }
}

void viewGroupHistory(TcpClient& client, const json& g) {
    system("clear");
    pushOpt("======================\n");
    showGroupHistory(client, g, 1);
    pushOpt("======================\n");
    pushOpt("聊天记录: " + std::string(g["name"]) + "\n");
    pushOpt("（输入任意内容返回）\n");
    setPrefix("返回:");
    popIpt();
}