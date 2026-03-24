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
#include <QJsonArray>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QTimer>
#include <QCoreApplication>
#include <QTextBlock>

#include <QDebug> // 确保包含 QDebug

// 匹配头文件中的新构造函数声明
MainWindow::MainWindow(NetworkClient* clientPtr, QWidget *parent)
    : QMainWindow(parent)
    , networkClient(clientPtr)
    , networkPollTimer(new QTimer(this))
    , waitingForResponse(false)
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


    connect(newPersonaButton, &QPushButton::clicked, this, &MainWindow::onNewPersonaClicked);
    connect(sendButton, &QPushButton::clicked, this, &MainWindow::onSendClicked);
    connect(personaListWidget, &QListWidget::itemClicked, this, &MainWindow::onPersonaSelected);
    
    connect(networkClient, &NetworkClient::messageReceived,
            this, &MainWindow::onServerResponseReceived);
    
    connect(networkClient, &NetworkClient::connectionStateChanged,
            this, &MainWindow::onConnectionStateChanged);
    
    // 设置轮询定时器，每100毫秒检查一次网络响应
    connect(networkPollTimer, &QTimer::timeout, this, &MainWindow::checkNetworkResponse);
    networkPollTimer->setInterval(100);  // 100ms
    networkPollTimer->start();
    
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
            int personaId = db->addPersona(personaName, personaDesc);
            if (personaId > 0) {
                // 添加成功，创建列表项并绑定ID
                QListWidgetItem* item = new QListWidgetItem(personaName, personaListWidget);
                item->setData(Qt::UserRole, personaId); // 设置人设ID
                
                // 自动选中新添加的项
                personaListWidget->setCurrentItem(item);
                onPersonaSelected(item);
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

    qDebug("选中人设: %s (ID: %s)", qUtf8Printable(currentPersonaName), qUtf8Printable(currentPersonaId)); // 调试输出

    // 加载历史聊天记录
    loadChatHistory(personaId);

    // 启用输入
    messageInput->setEnabled(true);
    sendButton->setEnabled(true);
    messageInput->setFocus(); // 将焦点设置到输入框
}

void MainWindow::loadChatHistory(int personaId) {
    // 查询数据库获取聊天记录
    auto chatHistory = db->getChatHistory(personaId);
    
    if (chatHistory.isEmpty()) {
        // 如果没有聊天记录，显示欢迎消息
        addChatMessage(currentPersonaName, QString("你好！我是%1，我们可以开始聊天了。").arg(currentPersonaName), false, false);
        return;
    }
    
    // 显示聊天记录
    for (const auto &record : chatHistory) {
        QString sender = record[0];
        QString message = record[1];
        bool isUserMessage = (sender == "你");
        
        // 添加历史消息，但不保存到数据库
        addChatMessage(sender, message, isUserMessage, false);
    }
}

void MainWindow::onSendClicked()
{
    QString message = messageInput->text().trimmed();

    if (message.isEmpty() || currentPersonaId.isEmpty()) {
        return;
    }

    // 发送消息并保存到数据库
    addChatMessage("你", message, true, true);
    sendMessageToServer(currentPersonaId.toInt(), message);
    
    messageInput->clear();
    messageInput->setFocus();

    sendButton->setEnabled(false);
    waitingForResponse = true;

    // 添加一个带有特殊ID的思考中提示，方便后续删除
    chatDisplayArea->append("<div id='thinking-indicator' align='left' style='color: gray;'><i>AI 正在思考中...</i></div>");
    chatDisplayArea->verticalScrollBar()->setValue(chatDisplayArea->verticalScrollBar()->maximum());
}

void MainWindow::onServerResponseReceived(const std::string& serverResponse)
{
    qDebug() << "[MainWindow] Server response received, length:" << serverResponse.size() << "bytes";
    
    // 重置等待响应的状态
    waitingForResponse = false;
    
    QByteArray data = QByteArray::fromStdString(serverResponse);
    QString jsonString = QString::fromUtf8(data);
        
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonString.toUtf8(), &parseError);

    QString aiResponseMessage = "[错误：无法解析服务器响应]";

    if(parseError.error == QJsonParseError::NoError && doc.isObject()) {
        QJsonObject obj = doc.object();
        
        if(obj.contains("response") && obj.value("response").isString()){
            aiResponseMessage = obj.value("response").toString();
        }
        else{
            aiResponseMessage = "[错误：服务器响应格式不正确]";
        }
    } else {
        qDebug() << "[MainWindow] JSON parse error:" << parseError.errorString() 
                << "at offset" << parseError.offset;
    }

    // 完全清除思考中提示 - 使用QTextDocument的API查找和删除
    QTextDocument *doc2 = chatDisplayArea->document();
    QTextCursor cursor(doc2);
    cursor.beginEditBlock();
    
    // 从文档开始查找
    cursor.movePosition(QTextCursor::Start);
    while (!cursor.isNull() && !cursor.atEnd()) {
        // 使用QTextBlock查找包含指定文本的行
        QTextBlock block = cursor.block();
        if (block.text().contains("AI 正在思考中")) {
            // 选中整个块
            cursor.select(QTextCursor::BlockUnderCursor);
            // 删除所选内容
            cursor.removeSelectedText();
            // 如果删除后出现空行，也删除空行
            if (cursor.block().text().isEmpty()) {
                cursor.deleteChar();
            }
            break;
        }
        cursor.movePosition(QTextCursor::NextBlock);
    }
    cursor.endEditBlock();

    // 添加AI回复并保存到数据库
    addChatMessage(currentPersonaName, aiResponseMessage, false, true);

    // 重新启用发送按钮
    sendButton->setEnabled(true);
    messageInput->setFocus();
    
    // 强制处理事件，确保UI更新
    QCoreApplication::processEvents();
}


void MainWindow::addChatMessage(const QString &sender, const QString &message, bool isUserMessage, bool saveToDatabase)
{
    // 使用HTML进行基本格式化(对齐，加粗发送者)
    QString formattedMessage;
    if (isUserMessage) {
        // 用户消息: 右对齐
        formattedMessage = QString("<div align='left'><b>%1:</b> %2</div>")
                               .arg(sender) // 发送者("你")
                               .arg(message.toHtmlEscaped()); // 转义消息中的HTML
    } else {
        // AI消息: 左对齐
        formattedMessage = QString("<div align='left'><b>%1:</b> %2</div>")
                               .arg(sender) // 发送者(AI人设名称)
                               .arg(message.toHtmlEscaped());
    }

    chatDisplayArea->append(formattedMessage); // 添加新段落

    // 保存消息到数据库
    if (saveToDatabase && !currentPersonaId.isEmpty()) {
        db->addMessage(currentPersonaId.toInt(), sender, message);
    }

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
        
        qDebug() << "加载人设:" << persona.second << "(ID:" << persona.first << ")";
    }
}

void MainWindow::sendMessageToServer(const int persona_id, const QString &message){
    auto personaInfo = db->getPersonaInfo(persona_id);
    if(personaInfo.first.isEmpty()){
        qWarning() << "获取人设信息失败";
        return;
    }

    // 获取最近的5条聊天记录（不包括当前新消息）
    QVector<QVector<QString>> recentMessages = db->getRecentChatHistory(persona_id, 5);
    
    // 检查聊天历史的最后一条消息是否与当前消息相同（排除重复）
    if (!recentMessages.isEmpty()) {
        // 如果最后一条是用户消息并且内容与当前消息相同，则移除它
        QVector<QString> lastMessage = recentMessages.last();
        if (lastMessage[0] == "你" && lastMessage[1] == message) {
            recentMessages.removeLast();
        }
    }
    
    // 计算历史记录总长度
    int historyLength = 0;
    for (const auto &record : recentMessages) {
        historyLength += record[1].length();
    }
    qDebug() << "历史聊天记录长度:" << historyLength << "字符";

    // 构建聊天历史数组
    QJsonArray chatHistoryArray;
    for (const auto &record : recentMessages) {
        QJsonObject messageObj;
        messageObj["sender"] = record[0];  // sender
        messageObj["content"] = record[1]; // message
        chatHistoryArray.append(messageObj);
    }

    // 使用Qt的JSON库处理
    QJsonObject rootObj;
    rootObj["personaId"] = persona_id;
    rootObj["persona"] = personaInfo.first;
    rootObj["personaDesc"] = personaInfo.second;
    rootObj["message"] = message;
    rootObj["chatHistory"] = chatHistoryArray;
    rootObj["historyLength"] = historyLength;

    // 生成JSON数据
    QJsonDocument doc(rootObj);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
    
    // 发送数据
    networkClient->sendMessage(jsonData.toStdString());
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

// 新增：检查网络响应
void MainWindow::checkNetworkResponse()
{
    if (waitingForResponse) {
        // 调用 NetworkClient 的 checkForResponse 方法
        networkClient->checkForResponse();
    }
}

void MainWindow::onConnectionStateChanged(bool connected) {
    if (connected) {
        addSystemMessage("服务器连接已重新建立");
        sendButton->setEnabled(true);
    } else {
        addSystemMessage("服务器连接已断开");
        // 不禁用发送按钮，因为我们有自动重连机制
    }
}

void MainWindow::addSystemMessage(const QString &message) {
    QString formattedMessage = QString("<div align='center' style='color: gray;'><i>%1</i></div>")
                                   .arg(message); // 居中显示系统消息
    chatDisplayArea->append(formattedMessage);
    
    // 自动滚动到底部
    chatDisplayArea->verticalScrollBar()->setValue(chatDisplayArea->verticalScrollBar()->maximum());
}
