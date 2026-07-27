#include "Friend.h"

Friend::Friend(int id,
               int user_id,
               int friend_id,
               int status,
               std::string remark)
    : m_id(id),
      m_user_id(user_id),
      m_friend_id(friend_id),
      m_status(status),
      m_remark(remark) {}

int Friend::getId() const {
    return m_id;
}

int Friend::getUserId() const {
    return m_user_id;
}

int Friend::getFriendId() const {
    return m_friend_id;
}

int Friend::getStatus() const {
    return m_status;
}

std::string Friend::getRemark() const {
    return m_remark;
}

void Friend::setId(int id) {
    m_id = id;
}

void Friend::setUserId(int user_id) {
    m_user_id = user_id;
}

void Friend::setFriendId(int friend_id) {
    m_friend_id = friend_id;
}

void Friend::setStatus(int status) {
    m_status = status;
}

void Friend::setRemark(const std::string& remark) {
    m_remark = remark;
}