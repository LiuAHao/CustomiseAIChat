#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString> // 包含QString头文件
#include <QTimer>  // 添加QTimer头文件
#include "../db/database.h"
#include "network/NetworkClient.h" // 包含头文件
// 前置声明以减少头文件依赖
class QSplitter;
class QListWidget;
class QTextEdit;
class QLineEdit;
class QPushButton;
class QVBoxLayout;
class QHBoxLayout;
class QWidget;
class QListWidgetItem; // 用于槽函数参数

// class NetworkClient; // 移除多余的前置声明

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    // 修改构造函数以接收 NetworkClient 指针
    MainWindow(NetworkClient* clientPtr, QWidget *parent = nullptr); // <-- 修改构造函数声明
    ~MainWindow();

private slots:
    // 处理UI事件的槽函数
    void onNewPersonaClicked();
    void onSendClicked();
    void onPersonaSelected(QListWidgetItem *item); // 列表选择槽函数
    void onServerResponseReceived(const std::string& serverResponse); // 服务器响应槽函数
    void showPersonaContextMenu(const QPoint &pos); // 添加右键菜单函数声明
    void checkNetworkResponse(); // 检查网络响应
    void onConnectionStateChanged(bool connected); // 处理连接状态变化

private:
    // 辅助函数：初始化UI
    void setupUI();
    // 辅助函数：添加消息到聊天显示区域
    void addChatMessage(const QString &sender, const QString &message, bool isUserMessage);
    void addSystemMessage(const QString &message); // 添加系统消息
    void deletePersona(int personaId); // 添加删除人设函数声明 
    void loadPersonasFromDatabase();
    void sendMessageToServer(const int persona_id, const QString &message);
    QString receiveMessageFromServer();

    // 右键菜单
    QMenu *personaContextMenu;
    QAction *deletePersonaAction;

    //数据库
    Database *db;
    //网络 (成员变量保持不变)
    NetworkClient *networkClient;
    
    // 添加定时器用于轮询网络响应
    QTimer *networkPollTimer;
    bool waitingForResponse; // 是否正在等待响应

    // --- UI组件 ---
    QSplitter *mainSplitter;

    // 左侧面板(人设列表)
    QWidget *leftPaneWidget;
    QVBoxLayout *leftPaneLayout;
    QPushButton *newPersonaButton;
    QListWidget *personaListWidget;

    // 右侧面板(聊天区域)
    QWidget *rightPaneWidget;
    QVBoxLayout *rightPaneLayout;
    QTextEdit *chatDisplayArea; // 显示聊天历史
    QWidget *inputAreaWidget;    // 输入区域容器
    QHBoxLayout *inputAreaLayout;
    QLineEdit *messageInput;    // 用户输入框
    QPushButton *sendButton;

    // --- 状态 ---
    QString currentPersonaId;   // 当前选中人设ID
    QString currentPersonaName; // 当前选中人设名称
    
};

#endif // MAINWINDOW_H
