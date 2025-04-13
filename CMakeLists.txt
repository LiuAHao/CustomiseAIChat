cmake_minimum_required(VERSION 3.5)
project(AIChat LANGUAGES CXX)

# 查找Qt6组件，添加Sql模块
find_package(Qt6 REQUIRED COMPONENTS Core Sql Widgets)

# 添加数据库库
add_library(AIChatDB STATIC
    src/db/database.cpp
    src/db/database.h
)

# 链接Qt SQL模块到数据库库
target_link_libraries(AIChatDB PRIVATE Qt6::Sql)

# 添加UI可执行文件
add_executable(ui
    src/ui/main.cpp
    src/ui/mainwindow.cpp
    src/ui/mainwindow.h
)

# 链接数据库库和Qt Widgets模块到UI可执行文件
target_link_libraries(ui PRIVATE 
    AIChatDB
    Qt6::Widgets
    Qt6::Sql
)

# 添加包含路径
target_include_directories(ui PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/db
)