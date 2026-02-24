cmake_minimum_required(VERSION 3.10)
project(CustomiseAIChat LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ===== 网络库（静态库） =====
add_subdirectory(src/net)

# ===== 后端服务器 =====
add_subdirectory(src/server)

# ===== Qt客户端（可选，需要Qt环境） =====
option(BUILD_CLIENT "构建Qt客户端" OFF)
if(BUILD_CLIENT)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)
    set(CMAKE_AUTOUIC ON)

    find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Sql)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Sql)

    add_subdirectory(src/ui)
endif()