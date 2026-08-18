cmake_minimum_required(VERSION 3.10)

project(client)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(client
    main.cc
    common/common.cc
    net/TcpClient.cc
    net/net.cc
    page/auth.cc
    page/friend.cc
    page/group.cc
    page/settings.cc
)

target_include_directories(client PRIVATE ${PROJECT_SOURCE_DIR})

target_link_libraries(client PRIVATE readline pthread)