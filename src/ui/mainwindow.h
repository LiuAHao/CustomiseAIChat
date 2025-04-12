#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString> // 包含QString头文件

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

class MainWindow : public QMainWindow
{
    Q_OBJECT // 信号槽机制必需

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // 处理UI事件的槽函数
    void onNewPersonaClicked();
    void onSendClicked();
    void onPersonaSelected(QListWidgetItem *item); // 列表选择槽函数

private:
    // 辅助函数：初始化UI
    void setupUI();
    // 辅助函数：添加消息到聊天显示区域
    void addChatMessage(const QString &sender, const QString &message, bool isUserMessage);

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
