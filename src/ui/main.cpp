#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv); // 创建应用实例
    MainWindow w;              // 创建主窗口实例
    w.show();                  // 显示主窗口
    return a.exec();           // 启动应用事件循环
}
