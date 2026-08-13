#include "User.h"

User::User(int id,
           std::string name,
           std::string email,
           std::string password,
           int state)
    : m_id(id),
      m_name(name),
      m_email(email),
      m_password(password) {}

int User::getId() const {
    return m_id;
}

std::string User::getName() const {
    return m_name;
}

std::string User::getEmail() const {
    return m_email;
}

std::string User::getPassword() const {
    return m_password;
}

void User::setId(int id) {
    m_id = id;
}

void User::setName(const std::string& name) {
    m_name = name;
}

void User::setEmail(const std::string& email) {
    m_email = email;
}

void User::setPassword(const std::string& password) {
    m_password = password;
}