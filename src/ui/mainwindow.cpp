#include "mainwindow.h"

#include <QSplitter>
#include <QListWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QInputDialog> // 用于获取新人设名称
#include <QMessageBox>  // 用于显示反馈/错误
#include <QListWidgetItem> // 包含QListWidgetItem头文件
#include <QScrollBar>   // 用于自动滚动到底部
#include <QDialog>
#include <QFormLayout>
#include <QDialogButtonBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUI();

    // --- 连接信号与槽 ---
    connect(newPersonaButton, &QPushButton::clicked, this, &MainWindow::onNewPersonaClicked);
    connect(sendButton, &QPushButton::clicked, this, &MainWindow::onSendClicked);
    connect(personaListWidget, &QListWidget::itemClicked, this, &MainWindow::onPersonaSelected);

    // --- 初始状态(示例) ---
    // 实际应用中应从数据库加载人设列表
    personaListWidget->addItem("原版Deepseek");
    
    // 如果列表不为空则默认选中第一项
    if (personaListWidget->count() > 0) {
        personaListWidget->setCurrentRow(0); // 选中第一项
        onPersonaSelected(personaListWidget->item(0)); // 触发选择逻辑
    } else {
        chatDisplayArea->setPlaceholderText("请先新建或选择一个人设");
        messageInput->setEnabled(false);
        sendButton->setEnabled(false);
    }
}

MainWindow::~MainWindow()
{
    // Qt会自动删除有父对象的子控件
}

void MainWindow::setupUI()
{
    // --- 左侧面板(人设列表) ---
    leftPaneWidget = new QWidget(this);
    leftPaneLayout = new QVBoxLayout(leftPaneWidget); // 设置布局父对象

    newPersonaButton = new QPushButton("新建人设", leftPaneWidget);
    personaListWidget = new QListWidget(leftPaneWidget);

    leftPaneLayout->addWidget(newPersonaButton);
    leftPaneLayout->addWidget(personaListWidget);
    leftPaneWidget->setLayout(leftPaneLayout); // 应用布局

    // --- 右侧面板(聊天区域) ---
    rightPaneWidget = new QWidget(this);
    rightPaneLayout = new QVBoxLayout(rightPaneWidget);

    chatDisplayArea = new QTextEdit(rightPaneWidget);
    chatDisplayArea->setReadOnly(true); // 用户不能直接编辑聊天历史
    chatDisplayArea->setStyleSheet("QTextEdit {"
                                 "border: 1px solid #ccc;"  // 灰色边框
                                 "border-radius: 5px;"      // 圆角
                                 "padding: 5px;"           // 内边距
                                 "background-color: white;" // 白色背景
                                 "}");

    // 输入区域(输入框 + 发送按钮)
    inputAreaWidget = new QWidget(rightPaneWidget);
    inputAreaLayout = new QHBoxLayout(inputAreaWidget);
    messageInput = new QLineEdit(inputAreaWidget);
    messageInput->setPlaceholderText("输入消息..."); // 占位文本
    sendButton = new QPushButton("发送", inputAreaWidget);
    inputAreaLayout->addWidget(messageInput);
    inputAreaLayout->addWidget(sendButton);
    inputAreaLayout->setContentsMargins(0, 0, 0, 0); // 移除输入区域边距
    inputAreaWidget->setLayout(inputAreaLayout);

    rightPaneLayout->addWidget(chatDisplayArea);
    rightPaneLayout->addWidget(inputAreaWidget);
    rightPaneWidget->setLayout(rightPaneLayout);

    // --- 主分割器 ---
    mainSplitter = new QSplitter(Qt::Horizontal, this);
    mainSplitter->addWidget(leftPaneWidget);
    mainSplitter->addWidget(rightPaneWidget);

    // 设置初始大小(可选，根据需要调整)
    mainSplitter->setSizes(QList<int>() << 150 << 450); // 调整宽度: 左面板, 右面板

    // 将分割器设置为主窗口的中心控件
    setCentralWidget(mainSplitter);

    // --- 窗口设置 ---
    setWindowTitle("AI 聊天系统");
    resize(800, 600); // 设置初始窗口大小
}

// --- 槽函数实现 ---

void MainWindow::onNewPersonaClicked()
{
    // 创建自定义对话框
    QDialog dialog(this);
    dialog.setWindowTitle("新建人设");
    
    QFormLayout form(&dialog);
    
    // 添加名称输入框
    QLineEdit nameEdit;
    nameEdit.setFrame(true); // 确保有完整边框
    form.addRow("人设名称:", &nameEdit);
    
    // 添加描述输入框
    // 使用QTextEdit但添加样式表
    QTextEdit descEdit;
    descEdit.setStyleSheet("QTextEdit { border: 1px solid gray; border-radius: 2px; }");
    descEdit.setMinimumHeight(100);
    form.addRow("人设描述:", &descEdit);
    
    // 添加按钮
    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                               Qt::Horizontal, &dialog);
    buttonBox.button(QDialogButtonBox::Ok)->setText("确定"); // 设置中文按钮
    buttonBox.button(QDialogButtonBox::Cancel)->setText("取消");
    form.addRow(&buttonBox);
    
    // 连接按钮信号
    QObject::connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    
    // 显示对话框
    if (dialog.exec() == QDialog::Accepted) {
        QString personaName = nameEdit.text().trimmed();
        QString personaDesc = descEdit.toPlainText().trimmed();
        
        if (!personaName.isEmpty()) {
            // 将新人设添加到列表控件
            QListWidgetItem *newItem = new QListWidgetItem(personaName, personaListWidget);
            newItem->setData(Qt::UserRole + 1, personaDesc); // 存储描述
            personaListWidget->addItem(newItem);

            // 选中新创建的人设
            personaListWidget->setCurrentItem(newItem);
            onPersonaSelected(newItem);
        }
    }
}

void MainWindow::onPersonaSelected(QListWidgetItem *item)
{
    if (!item) return; // 列表不为空时不应发生，但作为良好实践保留

    currentPersonaName = item->text();
    // 如果存储了ID: currentPersonaId = item->data(Qt::UserRole).toString();
    currentPersonaId = currentPersonaName; // 本例中使用名称作为ID

    setWindowTitle(QString("AI 聊天系统 - 与 %1 对话中").arg(currentPersonaName));
    chatDisplayArea->clear(); // 清除之前的聊天记录

    // **占位:** 这里应该:
    // 1. 从后端请求'currentPersonaId'的聊天历史
    // 2. 用获取的历史记录填充'chatDisplayArea'
    qDebug("选中人设: %s", qUtf8Printable(currentPersonaName)); // 调试输出

    // 示例: 添加欢迎消息
    addChatMessage(currentPersonaName, QString("你好！我是%1，我们可以开始聊天了。").arg(currentPersonaName), false);

    // 启用输入
    messageInput->setEnabled(true);
    sendButton->setEnabled(true);
    messageInput->setFocus(); // 将焦点设置到输入框
}

void MainWindow::onSendClicked()
{
    QString message = messageInput->text().trimmed(); // 获取文本并去除首尾空格

    if (message.isEmpty() || currentPersonaId.isEmpty()) {
        // 不发送空消息或未选择人设时的消息
        return;
    }

    // 1. 立即显示用户消息(右对齐)
    addChatMessage("你", message, true); // 'true'表示用户消息

    // **占位:** 这里应该:
    // 2. 将'message'和'currentPersonaId'发送到后端/AI服务
    qDebug("发送消息给 %s: %s", qUtf8Printable(currentPersonaName), qUtf8Printable(message));

    messageInput->clear(); // 清空输入框
    messageInput->setFocus(); // 重新聚焦到输入框

    // **占位:** 这里应该:
    // 3. 异步等待来自后端的AI响应
    // 4. 当响应到达时，显示它(左对齐)
    // 示例: 模拟延迟后的AI响应(在实际应用中，应使用来自网络/工作线程的信号槽)
    // 在实际应用中不要像这样阻塞UI线程!
    // QTimer::singleShot(500, [this, message]() { // 模拟延迟
    //     QString aiResponse = QString("关于"%"1"，我的想法是... (模拟回复)").arg(message);
    //     addChatMessage(currentPersonaName, aiResponse, false);
    // });
    // 现在，我们只是立即添加一个占位响应:
    QString aiResponse = QString("模拟 AI 对“%1”的回复...").arg(message);
    addChatMessage(currentPersonaName, aiResponse, false); // 'false'表示AI消息
}

// --- 辅助函数 ---

void MainWindow::addChatMessage(const QString &sender, const QString &message, bool isUserMessage)
{
    // 使用HTML进行基本格式化(对齐，加粗发送者)
    QString formattedMessage;
    if (isUserMessage) {
        // 用户消息: 右对齐
        formattedMessage = QString("<div align='right'><b>%1:</b> %2</div>")
                               .arg(sender) // 发送者("你")
                               .arg(message.toHtmlEscaped()); // 转义消息中的HTML
    } else {
        // AI消息: 左对齐
        formattedMessage = QString("<div align='left'><b>%1:</b> %2</div>")
                               .arg(sender) // 发送者(AI人设名称)
                               .arg(message.toHtmlEscaped());
    }

    chatDisplayArea->append(formattedMessage); // 添加新段落

    // 自动滚动到底部
    chatDisplayArea->verticalScrollBar()->setValue(chatDisplayArea->verticalScrollBar()->maximum());
}
