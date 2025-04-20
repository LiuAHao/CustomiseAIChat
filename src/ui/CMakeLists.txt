cmake_minimum_required(VERSION 3.5)

project(ui LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Sql)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Sql)

# 定义源文件列表
set(SOURCES
    main.cpp
    mainwindow.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../db/database.cpp
    network/NetworkClient.cpp      # <-- 将 Network 改为 network
)

# 定义头文件列表
set(HEADERS
    mainwindow.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../db/database.h
    network/NetworkClient.h      # <-- 将 Network 改为 network
)

add_executable(ui
    ${SOURCES}
    ${HEADERS}
)

# 链接 Qt 库
target_link_libraries(ui PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Sql)

# 添加包含目录
target_include_directories(ui PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/network
    ${CMAKE_CURRENT_SOURCE_DIR}/../db
)

