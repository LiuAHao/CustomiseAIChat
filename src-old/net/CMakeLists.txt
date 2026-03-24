# 网络库静态库
file(GLOB NET_SOURCES "*.cpp")
file(GLOB NET_HEADERS "*.h")

add_library(net STATIC ${NET_SOURCES} ${NET_HEADERS})

target_include_directories(net PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(net PUBLIC pthread)
