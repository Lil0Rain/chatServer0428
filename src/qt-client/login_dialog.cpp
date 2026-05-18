#include "login_dialog.h"
#include "public.hpp"
#include "json.hpp"

#include <QFormLayout>
#include <QVBoxLayout>

using json = nlohmann::json;

LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QString::fromUtf8("登录 Chat"));
    setFixedSize(340, 300);
    setObjectName("loginDialog");

    _serverIp   = new QLineEdit("192.168.112.129");
    _serverPort = new QLineEdit("8000");
    _userId     = new QLineEdit;
    _password   = new QLineEdit;
    _password->setEchoMode(QLineEdit::Password);
    _statusLabel = new QLabel;
    _statusLabel->setObjectName("statusLabel");
    _loginBtn    = new QPushButton(QString::fromUtf8("登 录"));
    _loginBtn->setObjectName("primaryBtn");

    _serverIp->setObjectName("inputField");
    _serverPort->setObjectName("inputField");
    _userId->setObjectName("inputField");
    _password->setObjectName("inputField");

    _serverIp->setPlaceholderText(QString::fromUtf8("192.168.112.129"));
    _serverPort->setPlaceholderText("8000");
    _userId->setPlaceholderText(QString::fromUtf8("请输入用户ID"));
    _password->setPlaceholderText(QString::fromUtf8("请输入密码"));

    auto *form = new QFormLayout;
    form->setSpacing(10);
    form->addRow(QString::fromUtf8("服务器 IP"), _serverIp);
    form->addRow(QString::fromUtf8("端口"), _serverPort);
    form->addRow(QString::fromUtf8("用户 ID"), _userId);
    form->addRow(QString::fromUtf8("密码"), _password);

    auto *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(32, 24, 32, 24);
    vbox->addLayout(form);
    vbox->addSpacing(8);
    vbox->addWidget(_statusLabel);
    vbox->addSpacing(4);
    vbox->addWidget(_loginBtn);

    connect(_loginBtn, &QPushButton::clicked, this, &LoginDialog::onLoginClicked);

    _statusLabel->setText(QString::fromUtf8("欢迎使用 Chat，请登录"));
    _statusLabel->setAlignment(Qt::AlignCenter);
}

LoginDialog::~LoginDialog() = default;

void LoginDialog::onLoginClicked() {
    _loginBtn->setEnabled(false);
    _statusLabel->setStyleSheet("color: #757575;");
    _statusLabel->setText(QString::fromUtf8("正在连接服务器..."));

    QString ip   = _serverIp->text().trimmed();
    quint16 port = _serverPort->text().trimmed().toUShort();
    int userId   = _userId->text().trimmed().toInt();
    QString pwd  = _password->text();

    if (ip.isEmpty() || port == 0) {
        _statusLabel->setStyleSheet("color: #E53935;");
        _statusLabel->setText(QString::fromUtf8("请输入有效的服务器 IP 和端口"));
        _loginBtn->setEnabled(true);
        return;
    }
    if (userId == 0 || pwd.isEmpty()) {
        _statusLabel->setStyleSheet("color: #E53935;");
        _statusLabel->setText(QString::fromUtf8("请输入用户 ID 和密码"));
        _loginBtn->setEnabled(true);
        return;
    }

    if (_socket) {
        _socket->deleteLater();
    }
    _socket = new QTcpSocket(this);
    _socket->connectToHost(ip, port);

    if (!_socket->waitForConnected(5000)) {
        _statusLabel->setStyleSheet("color: #E53935;");
        _statusLabel->setText(QString::fromUtf8("连接服务器超时，请检查 IP 和端口"));
        _loginBtn->setEnabled(true);
        _socket->deleteLater();
        _socket = nullptr;
        return;
    }

    json js;
    js["msgid"]    = LOGIN_MSG;
    js["id"]       = userId;
    js["password"] = pwd.toStdString();

    std::string request = js.dump();
    _socket->write(request.c_str(), request.size());

    if (!_socket->waitForReadyRead(5000)) {
        _statusLabel->setStyleSheet("color: #E53935;");
        _statusLabel->setText(QString::fromUtf8("等待服务器响应超时"));
        _loginBtn->setEnabled(true);
        _socket->deleteLater();
        _socket = nullptr;
        return;
    }

    QByteArray data = _socket->readAll();
    json response = json::parse(data.toStdString());
    int errnoVal  = response["errno"].get<int>();

    if (errnoVal == 0) {
        _response = response;
        _statusLabel->setStyleSheet("color: #43A047;");
        _statusLabel->setText(QString::fromUtf8("登录成功！欢迎 %1")
                                  .arg(QString::fromStdString(response["name"])));
        accept();
    } else if (errnoVal == 1) {
        _statusLabel->setStyleSheet("color: #E53935;");
        _statusLabel->setText(QString::fromUtf8("账号或密码错误"));
        _loginBtn->setEnabled(true);
        _socket->deleteLater();
        _socket = nullptr;
    } else if (errnoVal == 2) {
        _statusLabel->setStyleSheet("color: #FB8C00;");
        _statusLabel->setText(QString::fromUtf8("账号已在其他设备登录"));
        _loginBtn->setEnabled(true);
        _socket->deleteLater();
        _socket = nullptr;
    } else {
        _statusLabel->setStyleSheet("color: #E53935;");
        _statusLabel->setText(QString::fromUtf8("未知错误"));
        _loginBtn->setEnabled(true);
        _socket->deleteLater();
        _socket = nullptr;
    }
}
