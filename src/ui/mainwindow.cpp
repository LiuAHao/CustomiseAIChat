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
#include <QLabel>
#include <QJsonObject>
#include <QJsonDocument>
#include <QMenu>
#include <QAction>
#include <QMessageBox>



#include <QDebug> // 确保包含 QDebug

// 匹配头文件中的新构造函数声明
MainWindow::MainWindow(NetworkClient* clientPtr, QWidget *parent)
    : QMainWindow(parent)
    , networkClient(clientPtr) // <-- 使用传入的指针初始化 networkClient
    // , networkClient(new NetworkClient(this)) // <-- 移除这行错误的初始化
{

    // 数据库初始化和 UI 设置保持不变
    db = new Database("chat.db");
    if(!db->initDatabase()){
        qDebug() << "数据库初始化失败";
        // 考虑更健壮的错误处理，例如抛出异常或显示错误消息
        // return; // 直接返回可能不是最佳选择，取决于应用需求
    } else {
        setupUI();  // 确保 UI 设置在数据库成功初始化后执行
        loadPersonasFromDatabase(); // 从数据库加载人设
    }

    // --- 连接信号与槽 (保持不变) ---
    connect(newPersonaButton, &QPushButton::clicked, this, &MainWindow::onNewPersonaClicked);
    connect(sendButton, &QPushButton::clicked, this, &MainWindow::onSendClicked);
    connect(personaListWidget, &QListWidget::itemClicked, this, &MainWindow::onPersonaSelected);
  
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
    // --- 主窗口基础设置 ---
    setStyleSheet("QMainWindow { background-color: #f5f5f5; }");

    // --- 左侧面板(人设列表) ---
    leftPaneWidget = new QWidget(this);
    leftPaneWidget->setStyleSheet("QWidget { background-color: #ffffff; border-radius: 8px; }");
    leftPaneLayout = new QVBoxLayout(leftPaneWidget);

    newPersonaButton = new QPushButton("新建人设", leftPaneWidget);
    newPersonaButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #4a6fa5;"
        "   color: white;"
        "   border: none;"
        "   padding: 8px;"
        "   border-radius: 4px;"
        "}"
        "QPushButton:hover { background-color: #3a5a8f; }"
    );

    personaListWidget = new QListWidget(leftPaneWidget);
    personaListWidget->setStyleSheet(
        "QListWidget {"
        "   background-color: #ffffff;"
        "   border: 1px solid #e0e0e0;"
        "   border-radius: 4px;"
        "}"
        "QListWidget::item {"
        "   padding: 8px;"
        "   border-bottom: 1px solid #f0f0f0;"
        "}"
        "QListWidget::item:hover { background-color: #f8f9fa; }"
        "QListWidget::item:selected {"
        "   background-color: #e1e9f7;"
        "   color: #333333;"  // 深灰色文字
        "   font-weight: bold;"  // 加粗显示
        "}"
    );

    // --- 右侧面板(聊天区域) ---
    rightPaneWidget = new QWidget(this);
    rightPaneWidget->setStyleSheet("QWidget { background-color: #ffffff; border-radius: 8px; }");
    rightPaneLayout = new QVBoxLayout(rightPaneWidget);

    chatDisplayArea = new QTextEdit(rightPaneWidget);
    chatDisplayArea->setStyleSheet(
        "QTextEdit {"
        "   background-color: #ffffff;"
        "   border: 1px solid #e0e0e0;"
        "   border-radius: 4px;"
        "   padding: 10px;"
        "}"
    );

    // 输入区域
    inputAreaWidget = new QWidget(rightPaneWidget);
    inputAreaLayout = new QHBoxLayout(inputAreaWidget);

    messageInput = new QLineEdit(inputAreaWidget);
    messageInput->setStyleSheet(
        "QLineEdit {"
        "   border: 1px solid #e0e0e0;"
        "   border-radius: 4px;"
        "   padding: 8px;"
        "}"
    );

    sendButton = new QPushButton("发送", inputAreaWidget);
    sendButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #4a6fa5;"
        "   color: white;"
        "   border: none;"
        "   padding: 8px 16px;"
        "   border-radius: 4px;"
        "}"
        "QPushButton:hover { background-color: #3a5a8f; }"
        "QPushButton:disabled { background-color: #cccccc; }"
    );

    
    // 设置列表项高度
    personaListWidget->setIconSize(QSize(24, 24));
    personaListWidget->setSpacing(2);
    
    // 启用右键菜单
    personaListWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(personaListWidget, &QListWidget::customContextMenuRequested, 
            this, &MainWindow::showPersonaContextMenu);
    
    leftPaneLayout->addWidget(newPersonaButton);
    leftPaneLayout->addWidget(personaListWidget);
    leftPaneWidget->setLayout(leftPaneLayout); // 应用布局

    // --- 右侧面板(聊天区域) ---
    rightPaneWidget = new QWidget(this);
    rightPaneWidget->setStyleSheet("QWidget { background-color: #ffffff; border-radius: 8px; }");
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
    mainSplitter->setStyleSheet("QSplitter::handle { background-color: #e0e0e0; }");
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
            if (db->addPersona(personaName, personaDesc)) {
                // 添加成功，更新UI
                personaListWidget->addItem(personaName);
            } else {
                QMessageBox::warning(this, "警告", "添加人设失败");
            }
        }
    }
}

void MainWindow::onPersonaSelected(QListWidgetItem *item)
{
    if (!item) return; // 列表不为空时不应发生，但作为良好实践保留

    // 获取存储在item中的人设ID
    int personaId = item->data(Qt::UserRole).toInt();
    
    // 直接从列表项获取文本作为人设名称
    currentPersonaName = item->text();
    
    // 如果名称为空，则从数据库获取
    if (currentPersonaName.isEmpty()) {
        // 根据ID从数据库获取人设名称
        auto personas = db->getAllPersonas();
        for (auto &persona : personas) {
            if (persona.first == personaId) {
                currentPersonaName = persona.second;
                break;
            }
        }
    }
    
    // 设置当前人设ID
    currentPersonaId = QString::number(personaId);

    setWindowTitle(QString("AI 聊天系统 - 与 %1 对话中").arg(currentPersonaName));
    chatDisplayArea->clear(); // 清除之前的聊天记录

    // **占位:** 这里应该:
    // 1. 从后端请求'currentPersonaId'的聊天历史
    // 2. 用获取的历史记录填充'chatDisplayArea'
    qDebug("选中人设: %s (ID: %s)", qUtf8Printable(currentPersonaName), qUtf8Printable(currentPersonaId)); // 调试输出

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

    // 2. 发送消息到后端
    int personaId = currentPersonaId.toInt();
    sendMessageToServer(personaId, message);
    
    qDebug("发送消息给 %s: %s", qUtf8Printable(currentPersonaName), qUtf8Printable(message));

    messageInput->clear(); // 清空输入框
    messageInput->setFocus(); // 重新聚焦到输入框

    QString aiResponse = receiveMessageFromServer();
    addChatMessage(currentPersonaName, aiResponse, false);
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

void MainWindow::loadPersonasFromDatabase(){
    auto personas = db->getAllPersonas();
    personaListWidget->clear(); // 清空列表
    
    for(auto &persona : personas){
        // 创建列表项
        QListWidgetItem* item = new QListWidgetItem(persona.second, personaListWidget);
        
        // 存储人设ID到item的数据中
        item->setData(Qt::UserRole, persona.first);
    }
}

void MainWindow::sendMessageToServer(const int persona_id, const QString &message){
    auto personaInfo = db->getPersonaInfo(persona_id);
    if(personaInfo.first.isEmpty()){
        qWarning() << "获取人设信息失败";
        return;
    }

    QJsonObject messageObj;
    messageObj.insert("personaId", persona_id);
    messageObj.insert("persona", personaInfo.first);
    messageObj.insert("personaDesc", personaInfo.second);
    messageObj.insert("message", message);

    QJsonDocument doc(messageObj);
    QString jsonString = doc.toJson(QJsonDocument::Compact);

    QByteArray data = jsonString.toUtf8();

    // 将 QByteArray 转换为 std::string 再发送
    networkClient->sendMessage(data.toStdString()); // <-- 修改这里

}

QString MainWindow::receiveMessageFromServer(){
    // 接收 std::string
    std::string received_str = networkClient->receiveMessage();

    // 将 std::string 转换为 QByteArray
    QByteArray data = QByteArray::fromStdString(received_str);

    // 后续处理 QByteArray (解析 JSON) 保持不变
    QString jsonString = QString::fromUtf8(data);
    QJsonDocument doc = QJsonDocument::fromJson(jsonString.toUtf8());
    QJsonObject obj = doc.object();
    QString aiResponse = obj.value("response").toString();
    return aiResponse;
}

// 添加右键菜单显示函数
void MainWindow::showPersonaContextMenu(const QPoint &pos)
{
    QListWidgetItem *item = personaListWidget->itemAt(pos);
    if (!item) return;
    
    QMenu contextMenu(tr("人设操作"), this);
    
    QAction *deleteAction = new QAction(tr("删除"), this);
    connect(deleteAction, &QAction::triggered, [=]() {
        int personaId = item->data(Qt::UserRole).toInt();
        deletePersona(personaId);
    });
    
    contextMenu.addAction(deleteAction);
    contextMenu.exec(personaListWidget->mapToGlobal(pos));
}

// 将删除逻辑移到单独的函数中
void MainWindow::deletePersona(int personaId)
{
    // 确认对话框
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "确认删除", 
                                 "确定要删除这个人设吗？相关的所有聊天记录也会被删除。",
                                 QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        // 调用数据库删除函数
        if (db->deletePersona(personaId)) {
            // 删除成功，重新加载人设列表
            loadPersonasFromDatabase();
            
            // 如果当前选中的人设被删除，清空聊天区域
            if (currentPersonaId.toInt() == personaId) {
                currentPersonaId = "";
                currentPersonaName = "";
                chatDisplayArea->clear();
                chatDisplayArea->setPlaceholderText("请选择一个人设开始聊天");
                messageInput->setEnabled(false);
                sendButton->setEnabled(false);
                setWindowTitle("AI 聊天系统");
            }
        } else {
            QMessageBox::warning(this, "错误", "删除人设失败");
        }
    }
}
