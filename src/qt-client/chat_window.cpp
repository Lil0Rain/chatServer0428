#include "chat_window.h"
#include "public.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QInputDialog>
#include <QMessageBox>
#include <QDateTime>
#include <QScrollBar>

using json = nlohmann::json;

ChatWindow::ChatWindow(QTcpSocket *socket, const json &loginResponse, QWidget *parent)
    : QWidget(parent), _socket(socket)
{
    parseLoginResponse(loginResponse);
    setupUi();

    connect(_socket, &QTcpSocket::readyRead, this, &ChatWindow::onReadyRead);
    connect(_socket, &QTcpSocket::disconnected, this, &ChatWindow::onDisconnected);
}

ChatWindow::~ChatWindow() {
    _socket->deleteLater();
}

void ChatWindow::parseLoginResponse(const json &resp) {
    _myId   = resp["id"].get<int>();
    _myName = QString::fromStdString(resp["name"]);

    if (resp.contains("friend")) {
        for (auto &f : resp["friend"]) {
            json fj = json::parse(f.get<std::string>());
            FriendInfo info;
            info.id    = fj["id"];
            info.name  = QString::fromStdString(fj["name"]);
            info.state = QString::fromStdString(fj["state"]);
            _friends.append(info);
        }
    }

    if (resp.contains("groups")) {
        for (auto &g : resp["groups"]) {
            json gj = json::parse(g.get<std::string>());
            GroupInfo info;
            info.id   = gj["id"];
            info.name = QString::fromStdString(gj["groupname"]);
            info.desc = QString::fromStdString(gj["groupdesc"]);
            _groups.append(info);
        }
    }

    if (resp.contains("offlinemsg")) {
        for (auto &m : resp["offlinemsg"]) {
            json mj = json::parse(m.get<std::string>());
            int msgid = mj["msgid"].get<int>();
            int targetId = -1;
            if (msgid == ONE_CHAT_MSG) {
                targetId = mj["id"].get<int>();
            } else if (msgid == GROUP_CHAT_MSG) {
                targetId = mj["groupid"].get<int>();
            }
            if (targetId > 0) {
                QString line = makeChatHtml(mj["id"].get<int>(),
                    QString::fromStdString(mj["name"]),
                    QString::fromStdString(mj["message"]),
                    QString::fromStdString(mj.value("time", "")));
                _chatHistory[targetId].append(line);
            }
        }
    }
}

void ChatWindow::setupUi() {
    setWindowTitle(QString::fromUtf8("Chat - ") + _myName);
    resize(780, 520);
    setMinimumSize(600, 400);

    // ==== 顶部栏 ====
    auto *topBar = new QHBoxLayout;
    _userInfoLabel = new QLabel(
        QString::fromUtf8("<b style='font-size:15px;'>%1</b> &nbsp; #%2")
            .arg(_myName).arg(_myId));
    _userInfoLabel->setObjectName("userInfo");

    _connectionLabel = new QLabel(QString::fromUtf8("🟢 已连接"));
    _connectionLabel->setObjectName("connectionStatus");

    auto *logoutBtn = new QPushButton(QString::fromUtf8("注销"));
    logoutBtn->setObjectName("logoutBtn");

    topBar->addWidget(_userInfoLabel);
    topBar->addStretch();
    topBar->addWidget(_connectionLabel);
    topBar->addWidget(logoutBtn);
    topBar->setContentsMargins(12, 8, 12, 8);

    // ==== 左侧：好友 + 群组列表 ====
    auto *leftWidget = new QWidget;
    auto *leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(0);

    auto *friendHeader = new QLabel(QString::fromUtf8(" 好友"));
    friendHeader->setObjectName("sectionHeader");

    _friendList = new QListWidget;
    _friendList->setObjectName("contactList");
    for (auto &f : _friends) {
        QString text = f.state == "online"
            ? QString::fromUtf8("● %1").arg(f.name)
            : QString::fromUtf8("○ %1").arg(f.name);
        auto *item = new QListWidgetItem(text);
        item->setData(Qt::UserRole, f.id);
        item->setData(Qt::UserRole + 1, false); // not a group
        _friendList->addItem(item);
    }

    auto *groupHeader = new QLabel(QString::fromUtf8(" 群组"));
    groupHeader->setObjectName("sectionHeader");

    _groupList = new QListWidget;
    _groupList->setObjectName("contactList");
    for (auto &g : _groups) {
        auto *item = new QListWidgetItem(QString::fromUtf8("📝 %1").arg(g.name));
        item->setData(Qt::UserRole, g.id);
        item->setData(Qt::UserRole + 1, true); // is a group
        _groupList->addItem(item);
    }

    // 操作按钮行
    _addFriendBtn   = new QPushButton(QString::fromUtf8("+ 添加好友"));
    _addGroupBtn    = new QPushButton(QString::fromUtf8("+ 加入群组"));
    _createGroupBtn = new QPushButton(QString::fromUtf8("+ 创建群组"));
    _addFriendBtn->setObjectName("smallBtn");
    _addGroupBtn->setObjectName("smallBtn");
    _createGroupBtn->setObjectName("smallBtn");

    auto *btnRow = new QHBoxLayout;
    btnRow->addWidget(_addFriendBtn);
    btnRow->addWidget(_addGroupBtn);
    btnRow->addWidget(_createGroupBtn);
    btnRow->setContentsMargins(4, 4, 4, 4);

    leftLayout->addWidget(friendHeader);
    leftLayout->addWidget(_friendList, 2);
    leftLayout->addWidget(groupHeader);
    leftLayout->addWidget(_groupList, 2);
    leftLayout->addLayout(btnRow);

    // ==== 右侧：聊天区域 ====
    auto *rightWidget = new QWidget;
    auto *rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(8, 0, 8, 8);
    rightLayout->setSpacing(4);

    _chatTitle = new QLabel(QString::fromUtf8("欢迎使用 Chat，请选择好友或群组开始聊天"));
    _chatTitle->setObjectName("chatTitle");

    _chatDisplay = new QTextBrowser;
    _chatDisplay->setObjectName("chatDisplay");
    _chatDisplay->setOpenExternalLinks(false);

    auto *inputRow = new QHBoxLayout;
    _chatInput = new QTextEdit;
    _chatInput->setObjectName("chatInput");
    _chatInput->setMaximumHeight(72);
    _chatInput->setPlaceholderText(QString::fromUtf8("输入消息..."));

    _sendBtn = new QPushButton(QString::fromUtf8("发送"));
    _sendBtn->setObjectName("sendBtn");
    _sendBtn->setFixedSize(64, 72);

    inputRow->addWidget(_chatInput);
    inputRow->addWidget(_sendBtn);

    rightLayout->addWidget(_chatTitle);
    rightLayout->addWidget(_chatDisplay, 1);
    rightLayout->addLayout(inputRow);

    // ==== QSplitter ====
    _splitter = new QSplitter(Qt::Horizontal);
    _splitter->setObjectName("mainSplitter");
    _splitter->addWidget(leftWidget);
    _splitter->addWidget(rightWidget);
    _splitter->setStretchFactor(0, 2);
    _splitter->setStretchFactor(1, 5);
    _splitter->setHandleWidth(2);

    // ==== 主布局 ====
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->addLayout(topBar);
    mainLayout->addWidget(_splitter, 1);

    // ==== 连接信号 ====
    connect(_friendList, &QListWidget::itemDoubleClicked, this, &ChatWindow::onFriendDoubleClicked);
    connect(_groupList, &QListWidget::itemDoubleClicked, this, &ChatWindow::onGroupDoubleClicked);
    connect(_sendBtn, &QPushButton::clicked, this, &ChatWindow::onSendClicked);
    connect(_addFriendBtn, &QPushButton::clicked, this, &ChatWindow::onAddFriend);
    connect(_addGroupBtn, &QPushButton::clicked, this, &ChatWindow::onAddGroup);
    connect(_createGroupBtn, &QPushButton::clicked, this, &ChatWindow::onCreateGroup);
    connect(logoutBtn, &QPushButton::clicked, this, &ChatWindow::onLogout);

}

QString ChatWindow::makeChatHtml(int senderId, const QString &senderName,
                                   const QString &message, const QString &timeStr) {
    QString color;
    QString align;
    QString label;

    if (senderId == _myId) {
        color = "#1976D2";
        align = "right";
        label = QString::fromUtf8("我");
    } else {
        color = "#43A047";
        align = "left";
        label = senderName;
    }

    return QString(
        "<div style='text-align:%1; margin:4px 8px;'>"
        "<span style='color:#757575;font-size:11px;'>%2 %3</span><br>"
        "<span style='display:inline-block;max-width:70%%;background:%4;color:#fff;"
        "padding:6px 12px;border-radius:12px;font-size:13px;'>%5</span>"
        "</div>")
        .arg(align, label, timeStr, color, message.toHtmlEscaped());
}

void ChatWindow::switchChatTarget(int targetId, bool isGroup) {
    _currentTargetId = targetId;
    _currentIsGroup  = isGroup;

    if (isGroup) {
        for (auto &g : _groups) {
            if (g.id == targetId) {
                _chatTitle->setText(QString::fromUtf8("群聊: %1").arg(g.name));
                break;
            }
        }
        // 清除未读标记
        for (int i = 0; i < _groupList->count(); ++i) {
            auto *item = _groupList->item(i);
            if (item->data(Qt::UserRole).toInt() == targetId) {
                QString t = item->text();
                t.replace(QString::fromUtf8(" 🔴"), "");
                item->setText(t);
                break;
            }
        }
    } else {
        for (auto &f : _friends) {
            if (f.id == targetId) {
                _chatTitle->setText(QString::fromUtf8("与 %1 的对话").arg(f.name));
                break;
            }
        }
        // 清除未读标记
        for (int i = 0; i < _friendList->count(); ++i) {
            auto *item = _friendList->item(i);
            if (item->data(Qt::UserRole).toInt() == targetId) {
                QString t = item->text();
                t.replace(QString::fromUtf8(" 🔴"), "");
                item->setText(t);
                break;
            }
        }
    }
    loadChatHistory(targetId);
}

void ChatWindow::loadChatHistory(int targetId) {
    _chatDisplay->clear();
    if (_chatHistory.contains(targetId)) {
        for (auto &line : _chatHistory[targetId]) {
            _chatDisplay->append(line);
        }
        QScrollBar *sb = _chatDisplay->verticalScrollBar();
        sb->setValue(sb->maximum());
    }
}

void ChatWindow::updateFriendStatus(int friendId, const QString &state) {
    // 更新内存中的好友状态
    for (auto &f : _friends) {
        if (f.id == friendId) {
            f.state = state;
            break;
        }
    }
    // 更新 UI 列表中的 ●/○ 标记
    for (int i = 0; i < _friendList->count(); ++i) {
        auto *item = _friendList->item(i);
        if (item->data(Qt::UserRole).toInt() == friendId) {
            QString text = item->text();
            // 移除旧标记和未读标记
            text.remove(QString::fromUtf8("● "));
            text.remove(QString::fromUtf8("○ "));
            text.remove(QString::fromUtf8(" 🔴"));
            if (state == "online") {
                text.prepend(QString::fromUtf8("● "));
                item->setForeground(QColor("#212121"));
            } else {
                text.prepend(QString::fromUtf8("○ "));
                item->setForeground(QColor("#9E9E9E"));
            }
            item->setText(text);
            break;
        }
    }
}

// ============== Slots ==============

void ChatWindow::onReadyRead() {
    _buffer.append(_socket->readAll());

    while (!_buffer.isEmpty()) {
        // 跳过非 JSON 开头的数据
        int start = _buffer.indexOf('{');
        if (start < 0) { _buffer.clear(); return; }
        if (start > 0) _buffer.remove(0, start);

        // 大括号配对计数，定位一个完整 JSON 的结束位置
        int depth = 0;
        int end = -1;
        for (int i = 0; i < _buffer.size(); ++i) {
            if (_buffer[i] == '{') depth++;
            else if (_buffer[i] == '}') depth--;
            if (depth == 0) { end = i + 1; break; }
        }
        if (end < 0) return;  // JSON 未完整，等下次 readyRead

        QByteArray jsonBytes = _buffer.left(end);
        _buffer.remove(0, end);

        try {
            json js = json::parse(jsonBytes.toStdString());
            int msgid = js["msgid"].get<int>();

            if (msgid == ONE_CHAT_MSG) {
                int fromId      = js["id"].get<int>();
                QString name    = QString::fromStdString(js["name"]);
                QString message = QString::fromStdString(js["message"]);
                QString timeStr = QString::fromStdString(js.value("time", ""));

                QString line = makeChatHtml(fromId, name, message, timeStr);
                _chatHistory[fromId].append(line);

                if (_currentTargetId == fromId && !_currentIsGroup) {
                    _chatDisplay->append(line);
                    QScrollBar *sb = _chatDisplay->verticalScrollBar();
                    sb->setValue(sb->maximum());
                } else {
                    // 未读提示：在好友名后加标记
                    for (int i = 0; i < _friendList->count(); ++i) {
                        auto *item = _friendList->item(i);
                        if (item->data(Qt::UserRole).toInt() == fromId) {
                            QString text = item->text();
                            if (!text.endsWith(QString::fromUtf8(" 🔴")))
                                item->setText(text + QString::fromUtf8(" 🔴"));
                            break;
                        }
                    }
                }
            }
            else if (msgid == GROUP_CHAT_MSG) {
                int groupId     = js["groupid"].get<int>();
                int fromId      = js["id"].get<int>();
                QString name    = QString::fromStdString(js["name"]);
                QString message = QString::fromStdString(js["message"]);
                QString timeStr = QString::fromStdString(js.value("time", ""));

                QString line = makeChatHtml(fromId, name, message, timeStr);
                _chatHistory[groupId].append(line);

                if (_currentTargetId == groupId && _currentIsGroup) {
                    _chatDisplay->append(line);
                    QScrollBar *sb = _chatDisplay->verticalScrollBar();
                    sb->setValue(sb->maximum());
                } else {
                    for (int i = 0; i < _groupList->count(); ++i) {
                        auto *item = _groupList->item(i);
                        if (item->data(Qt::UserRole).toInt() == groupId) {
                            QString text = item->text();
                            if (!text.endsWith(QString::fromUtf8(" 🔴")))
                                item->setText(text + QString::fromUtf8(" 🔴"));
                            break;
                        }
                    }
                }
            }
            else if (msgid == FRIEND_STATUS_MSG) {
                int friendId = js["id"].get<int>();
                QString state = QString::fromStdString(js["state"]);
                updateFriendStatus(friendId, state);
            }
        } catch (const std::exception &) {
            // 解析失败则跳过这条，继续处理后续数据
        }
    }
}

void ChatWindow::onDisconnected() {
    _connectionLabel->setText(QString::fromUtf8("🔴 连接已断开"));
    _sendBtn->setEnabled(false);
    QMessageBox::warning(this, QString::fromUtf8("连接断开"),
                         QString::fromUtf8("与服务器的连接已断开"));
    close();
    emit loggedOut();
}

void ChatWindow::onFriendDoubleClicked(QListWidgetItem *item) {
    int targetId = item->data(Qt::UserRole).toInt();
    switchChatTarget(targetId, false);
}

void ChatWindow::onGroupDoubleClicked(QListWidgetItem *item) {
    int targetId = item->data(Qt::UserRole).toInt();
    switchChatTarget(targetId, true);
}

void ChatWindow::onSendClicked() {
    QString text = _chatInput->toPlainText().trimmed();
    if (text.isEmpty() || _currentTargetId <= 0) return;

    json js;
    if (_currentIsGroup) {
        js["msgid"]   = GROUP_CHAT_MSG;
        js["groupid"] = _currentTargetId;
    } else {
        js["msgid"] = ONE_CHAT_MSG;
        js["toid"]  = _currentTargetId;
    }
    js["id"]      = _myId;
    js["name"]    = _myName.toStdString();
    js["message"] = text.toStdString();

    std::string buf = js.dump();
    _socket->write(buf.c_str(), buf.size());

    QString line = makeChatHtml(_myId, _myName, text,
                   QDateTime::currentDateTime().toString("hh:mm"));
    _chatHistory[_currentTargetId].append(line);
    _chatDisplay->append(line);

    _chatInput->clear();
    QScrollBar *sb = _chatDisplay->verticalScrollBar();
    sb->setValue(sb->maximum());
}

void ChatWindow::onAddFriend() {
    bool ok;
    int friendId = QInputDialog::getInt(this, QString::fromUtf8("添加好友"),
                                        QString::fromUtf8("请输入好友 ID:"),
                                        0, 1, 999999, 1, &ok);
    if (!ok) return;

    json js;
    js["msgid"]    = ADD_FRIEND_MSG;
    js["id"]       = _myId;
    js["friendid"] = friendId;
    std::string buf = js.dump();
    _socket->write(buf.c_str(), buf.size());

    QMessageBox::information(this, QString::fromUtf8("提示"),
                             QString::fromUtf8("已发送添加好友请求"));
}

void ChatWindow::onAddGroup() {
    bool ok;
    int groupId = QInputDialog::getInt(this, QString::fromUtf8("加入群组"),
                                       QString::fromUtf8("请输入群组 ID:"),
                                       0, 1, 999999, 1, &ok);
    if (!ok) return;

    json js;
    js["msgid"]   = ADD_GROUP_MSG;
    js["id"]      = _myId;
    js["groupid"] = groupId;
    std::string buf = js.dump();
    _socket->write(buf.c_str(), buf.size());

    QMessageBox::information(this, QString::fromUtf8("提示"),
                             QString::fromUtf8("已发送加入群组请求"));
}

void ChatWindow::onCreateGroup() {
    bool ok;
    QString groupName = QInputDialog::getText(this, QString::fromUtf8("创建群组"),
                                              QString::fromUtf8("群组名称:"),
                                              QLineEdit::Normal, "", &ok);
    if (!ok || groupName.trimmed().isEmpty()) return;

    QString groupDesc = QInputDialog::getText(this, QString::fromUtf8("创建群组"),
                                              QString::fromUtf8("群组描述:"),
                                              QLineEdit::Normal, "", &ok);
    if (!ok) return;

    json js;
    js["msgid"]     = CREATE_GROUP_MSG;
    js["id"]        = _myId;
    js["groupname"] = groupName.trimmed().toStdString();
    js["groupdesc"] = groupDesc.trimmed().toStdString();
    std::string buf = js.dump();
    _socket->write(buf.c_str(), buf.size());

    QMessageBox::information(this, QString::fromUtf8("提示"),
                             QString::fromUtf8("已发送创建群组请求"));
}

void ChatWindow::onLogout() {
    json js;
    js["msgid"] = LOGOUT_MSG;
    js["id"]    = _myId;
    std::string buf = js.dump();
    _socket->write(buf.c_str(), buf.size());

    close();
    emit loggedOut();
}
