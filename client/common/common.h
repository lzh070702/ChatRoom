#pragma once

#include <nlohmann/json.hpp>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>
#include <vector>

class TcpClient;
using json = nlohmann::json;

// 全局状态
extern std::atomic<bool> g_running;
extern std::mutex g_opt_mtx;
extern std::queue<std::string> g_opt_que;
extern int g_opt_efd;
extern std::mutex g_ipt_mtx;
extern std::queue<std::string> g_ipt_que;
extern std::condition_variable g_ipt_cv;
extern std::mutex g_rsp_mtx;
extern std::queue<json> g_rsp_que;
extern std::condition_variable g_rsp_cv;
extern std::mutex g_ui_mutex;
extern std::string g_prefix;
extern json g_user;
extern std::mutex g_chat_mtx;
extern int g_chat_type;  // -1 无会话, 0 私聊, 1 群聊
extern int g_chat_id;

// UI 工具
void printOpt();
void pushOpt(const std::string& smsg);
std::vector<std::string> popOpt();
void pushIpt(const std::string& smsg);
std::string popIpt();
void pushRsp(const json& js);
json popRsp();
void setPrefix(const std::string& prefix);
void showTip(const std::string& msg);
bool confirm(const std::string& msg);
void setChatSession(int type, int id);
bool isCurrentSession(int type, int id);

// 编解码
std::string base64Encode(const std::string& data);
std::string base64Decode(const std::string& encoded);