#include "ChatService.h"
#include <iostream>

ChatService& ChatService::instance() {
    static ChatService service;
    return service;
}

void ChatService::handle(Connection* conn, const json& js) {
    int type = js["type"];
    auto it = m_handlers.find(type);
    if (it != m_handlers.end()) {
        it->second(conn, js);
    } else {
        // 错误处理
    }
}

ChatService::ChatService() {
    m_handlers[1] = [this](Connection* c, const json& j) { signIn(c, j); };
    m_handlers[2] = [this](Connection* c, const json& j) { signUp(c, j); };
}

void ChatService::signIn(Connection* conn, const json& js) {
    std::cout << "Sign in: " << js.dump(4) << std::endl;
}

void ChatService::signUp(Connection* conn, const json& js) {
    std::cout << "Sign up: " << js.dump(4) << std::endl;
}