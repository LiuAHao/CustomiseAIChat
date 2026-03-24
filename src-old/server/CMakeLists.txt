# 后端服务器
set(SERVER_SOURCES
    main.cpp
    ChatServer.cpp
    HttpClient.cpp
    config/ServerConfig.cpp
    db/Database.cpp
    service/UserService.cpp
    service/ChatService.cpp
    service/AIService.cpp
)

set(SERVER_HEADERS
    ChatServer.h
    HttpClient.h
    config/ServerConfig.h
    db/Database.h
    service/UserService.h
    service/ChatService.h
    service/AIService.h
    model/User.h
    model/Persona.h
    model/Message.h
)

add_executable(chat_server ${SERVER_SOURCES} ${SERVER_HEADERS})

target_include_directories(chat_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../net
)

target_link_libraries(chat_server PRIVATE
    net
    sqlite3
    curl
    jsoncpp
    ssl
    crypto
    pthread
)
