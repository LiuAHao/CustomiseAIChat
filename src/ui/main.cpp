#include "mainwindow.h"
#include "network/NetworkClient.h" // 确保包含了 NetworkClient 头文件
#include <QApplication>
#include <QDebug>
#include <cstdlib> // 为了 atoi
#include <cstdio>  // 为了 printf

int main(int argc, char *argv[])
{
    // 实例化 NetworkClient
    NetworkClient client;

    if (!client.connectToServer("192.168.232.129", 5085)) {
        qCritical() << "连接服务器失败";
        return -1; // 直接退出
    }

    // 连接成功后，再继续创建和显示 Qt 应用窗口
    qDebug() << "已连接到服务器" << argv[1] << ":" << argv[2];

    QApplication a(argc, argv); // 创建应用实例

    // 将 client 对象传递给 MainWindow 的构造函数
    MainWindow w(&client);      // <-- 修改这里，传递 client

    w.show();                  // 显示主窗口

    int result = a.exec();

    return result;
}
