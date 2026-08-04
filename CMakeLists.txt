cmake_minimum_required(VERSION 3.10)

project(ChatRoom)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SERVER_SRCS
    server/main.cc
    server/database/MySQL.cc
    server/database/Redis.cc
    server/email/EmailSender.cc
    server/model/FriendModel.cc
    server/model/GroupModel.cc
    server/model/MessageModel.cc
    server/model/User.cc
    server/model/UserModel.cc
    server/net/Connection.cc
    server/net/TcpServer.cc
    server/reactor/Reactor.cc
    server/service/ChatService.cc
)

add_executable(server ${SERVER_SRCS})

target_include_directories(server PRIVATE
    ${PROJECT_SOURCE_DIR}/server
    ${MYSQL_INCLUDE_DIR}
)

target_link_libraries(server PRIVATE
    mysqlclient
    hiredis
    curl
)

#mkdir build && cd build && cmake .. && make
#./server
