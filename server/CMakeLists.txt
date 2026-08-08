cmake_minimum_required(VERSION 3.10)

project(ChatRoom)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SERVER_SRCS
    main.cc
    database/MySQL.cc
    database/MySQLPool.cc
    database/Redis.cc
    email/EmailSender.cc
    model/FriendModel.cc
    model/GroupModel.cc
    model/MessageModel.cc
    model/User.cc
    model/UserModel.cc
    net/Connection.cc
    net/TcpServer.cc
    reactor/Reactor.cc
    service/ChatService.cc
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
    glog
    crypto
)