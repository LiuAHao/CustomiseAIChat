cmake_minimum_required(VERSION 3.10) # 或更高版本
project(AIChat LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_CXX_STANDARD 11) # 或更高版本
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找 Qt 包
find_package(Qt5 COMPONENTS Core Gui Widgets REQUIRED)

# 定义源文件列表
set(SOURCES
    src/ui/main.cpp
    src/ui/mainwindow.cpp
    src/ui/Network/NetworkClient.cpp # 确保这一行存在且正确
)


set(HEADERS
    src/ui/mainwindow.h
    src/ui/Network/NetworkClient.h   # 确保这一行存在且正确
)

# 定义 UI 文件 (如果使用 .ui 文件)
# set(UIS
#     src/ui/mainwindow.ui
# )

# 添加可执行文件目标
add_executable(AIChat ${SOURCES} ${HEADERS} ${UIS}) # 确保包含了 SOURCES 和 HEADERS

# 链接 Qt 库
target_link_libraries(AIChat PRIVATE Qt5::Core Qt5::Gui Qt5::Widgets)

# --- 网络库链接 (仅 Windows 需要) ---
# 如果你在 Windows 上编译，需要链接 Winsock 库
# if(WIN32)
#   target_link_libraries(AIChat PRIVATE ws2_32)
# endif()
# --- 网络库链接结束 ---


# 添加包含目录 (确保编译器能找到头文件)
target_include_directories(AIChat PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/Network # 添加 Network 目录
)