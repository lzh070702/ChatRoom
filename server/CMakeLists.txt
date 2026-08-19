cmake_minimum_required(VERSION 3.10)

project(ChatRoom)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

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

# ── 中转程序──
add_executable(transit ../transit/transit.cc)

target_include_directories(transit PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../transit
)

target_link_libraries(transit PRIVATE
    Threads::Threads
    crypto
)

# ── 前端托管程序──
add_executable(web ../web/main.cc)

target_include_directories(web PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../web
)

target_link_libraries(web PRIVATE
    Threads::Threads
)

add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/index.html
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/../web/index.html
        ${CMAKE_BINARY_DIR}/index.html
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../web/index.html
    COMMENT "Copy index.html into build directory"
)
add_custom_target(web_assets ALL DEPENDS ${CMAKE_BINARY_DIR}/index.html)