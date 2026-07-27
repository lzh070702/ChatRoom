#pragma once

#include <string>

class Friend {
   public:
    Friend(int id = -1,
           int user_id = -1,
           int friend_id = -1,
           int status = -1,
           std::string remark = "");
    // getter
    int getId() const;
    int getUserId() const;
    int getFriendId() const;
    int getStatus() const;
    std::string getRemark() const;
    // setter

    void setId(int id);
    void setUserId(int user_id);
    void setFriendId(int friend_id);
    void setStatus(int status);
    void setRemark(const std::string& remark);

   private:
    int m_id;
    int m_user_id;
    int m_friend_id;
    int m_status;
    std::string m_remark;
};