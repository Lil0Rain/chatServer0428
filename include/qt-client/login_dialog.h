#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QTcpSocket>
#include "json.hpp"

class LoginDialog : public QDialog {
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = nullptr);
    ~LoginDialog();

    QTcpSocket *socket() const { return _socket; }
    nlohmann::json loginResponse() const { return _response; }

private slots:
    void onLoginClicked();

private:
    QLineEdit   *_serverIp;
    QLineEdit   *_serverPort;
    QLineEdit   *_userId;
    QLineEdit   *_password;
    QLabel      *_statusLabel;
    QPushButton *_loginBtn;
    QTcpSocket  *_socket = nullptr;
    nlohmann::json _response;
};
