cmake_minimum_required(VERSION 3.10)

project(ChatRoom)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SERVER_SRCS
    server/main.cc
    # server/common/Logger.cc
    server/net/TcpServer.cc
    server/net/Connection.cc
    # server/protocol/JsonProtocol.cc
    server/reactor/Reactor.cc
)

add_executable(server ${SERVER_SRCS})

target_include_directories(server PRIVATE
    ${PROJECT_SOURCE_DIR}/server
)