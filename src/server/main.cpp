#include <signal.h>
#include <iostream>
#include <cstring>
#include "ChatServer.h"
#include "config/ServerConfig.h"
#include "db/Database.h"
#include "HttpClient.h"

ChatServer* g_server = nullptr;

void signalHandler(int sig) {
    std::cout << "\n[Main] 收到信号 " << sig << "，正在停止服务器..." << std::endl;

    if (g_server) {
        g_server->stop();
        delete g_server;
        g_server = nullptr;
    }

    Database::instance().close();
    HttpClient::instance().cleanup();

    exit(0);
}

void printUsage(const char* prog) {
    std::cout << "自定义人设AI聊天系统 - 后端服务器" << std::endl;
    std::cout << "用法:" << std::endl;
    std::cout << "  " << prog << " [配置文件路径]" << std::endl;
    std::cout << "  " << prog << " <IP> <端口>" << std::endl;
    std::cout << std::endl;
    std::cout << "示例:" << std::endl;
    std::cout << "  " << prog << " ./data/server.conf" << std::endl;
    std::cout << "  " << prog << " ./.env" << std::endl;
    std::cout << "  " << prog << " 0.0.0.0 5085" << std::endl;
    std::cout << std::endl;
}

int main(int argc, char* argv[]) {
    auto& config = ServerConfig::instance();

    // 默认先加载根目录 .env（不存在时忽略）
    config.loadFromFile(".env");

    // 解析命令行参数
    if (argc == 2) {
        // 通过配置文件覆盖默认配置（支持 .env / .conf）
        if (!config.loadFromFile(argv[1])) {
            std::cerr << "[Main] 加载配置文件失败，使用默认配置" << std::endl;
        }
    } else if (argc == 3) {
        // 通过命令行参数指定IP和端口
        config.ip = argv[1];
        config.port = static_cast<uint16_t>(atoi(argv[2]));
    } else if (argc > 3) {
        printUsage(argv[0]);
        return -1;
    }

    // 打印配置信息
    config.print();

    // 注册信号处理
    signal(SIGTERM, signalHandler);
    signal(SIGINT, signalHandler);

    // 初始化数据库
    if (!Database::instance().init(config.dbPath)) {
        std::cerr << "[Main] 数据库初始化失败，退出" << std::endl;
        return -1;
    }

    // 初始化HTTP客户端（用于AI API调用）
    if (!HttpClient::instance().init()) {
        std::cerr << "[Main] HTTP客户端初始化失败，退出" << std::endl;
        return -1;
    }

    // 创建并启动服务器
    std::cout << "[Main] 正在启动服务器 " << config.ip << ":" << config.port << " ..." << std::endl;

    g_server = new ChatServer(config.ip, config.port, config.ioThreads, config.workThreads);
    g_server->start();

    // start()是阻塞调用，到这里说明已停止
    delete g_server;
    g_server = nullptr;

    Database::instance().close();
    HttpClient::instance().cleanup();

    std::cout << "[Main] 服务器已停止" << std::endl;
    return 0;
}
