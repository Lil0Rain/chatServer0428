#pragma once

#include <QWidget>
#include <QTcpSocket>
#include <QListWidget>
#include <QTextBrowser>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QSplitter>
#include <QMap>
#include <QStringList>
#include "json.hpp"

struct FriendInfo {
    int id;
    QString name;
    QString state;
};

struct GroupInfo {
    int id;
    QString name;
    QString desc;
};

class ChatWindow : public QWidget {
    Q_OBJECT

public:
    ChatWindow(QTcpSocket *socket, const nlohmann::json &loginResponse, QWidget *parent = nullptr);
    ~ChatWindow();

signals:
    void loggedOut();

private slots:
    void onReadyRead();
    void onDisconnected();
    void onFriendDoubleClicked(QListWidgetItem *item);
    void onGroupDoubleClicked(QListWidgetItem *item);
    void onSendClicked();
    void onAddFriend();
    void onAddGroup();
    void onCreateGroup();
    void onLogout();

private:
    void setupUi();
    void parseLoginResponse(const nlohmann::json &resp);
    QString makeChatHtml(int senderId, const QString &senderName, const QString &message, const QString &timeStr);
    void switchChatTarget(int targetId, bool isGroup);
    void loadChatHistory(int targetId);
    void updateFriendStatus(int friendId, const QString &state);

    QTcpSocket *_socket;
    QByteArray  _buffer;  // TCP 粘包缓冲
    int _myId;
    QString _myName;

    // 好友/群组列表
    QList<FriendInfo> _friends;
    QList<GroupInfo>  _groups;

    // 聊天历史 {targetId → [htmlLines]}
    QMap<int, QStringList> _chatHistory;

    // 当前聊天对象
    int  _currentTargetId = -1;
    bool _currentIsGroup  = false;

    // UI 组件
    QSplitter   *_splitter;
    QListWidget *_friendList;
    QListWidget *_groupList;
    QTextBrowser *_chatDisplay;
    QTextEdit   *_chatInput;
    QPushButton *_sendBtn;
    QPushButton *_addFriendBtn;
    QPushButton *_addGroupBtn;
    QPushButton *_createGroupBtn;
    QLabel      *_chatTitle;
    QLabel      *_userInfoLabel;
    QLabel      *_connectionLabel;
};
