#include <QApplication>
#include "login_dialog.h"
#include "chat_window.h"

static const char *kStyleQSS = R"(
/* ==== 全局 ==== */
* {
    font-family: "Microsoft YaHei", "PingFang SC", "Noto Sans CJK SC", "WenQuanYi Micro Hei", sans-serif;
    font-size: 14px;
}

QWidget {
    background: #ECEFF1;
    color: #212121;
}

/* ==== 登录窗口 ==== */
QDialog#loginDialog {
    background: #FFFFFF;
    border-radius: 12px;
}

#loginDialog QLabel {
    background: transparent;
    color: #424242;
    font-size: 13px;
}

#loginDialog QLineEdit#inputField {
    padding: 8px 12px;
    border: 1px solid #E0E0E0;
    border-radius: 6px;
    background: #FAFAFA;
    color: #212121;
    font-size: 14px;
    min-height: 20px;
}

#loginDialog QLineEdit#inputField:focus {
    border-color: #1976D2;
    background: #FFFFFF;
}

/* ==== 主按钮 ==== */
QPushButton#primaryBtn {
    background: #1976D2;
    color: #FFFFFF;
    border: none;
    border-radius: 6px;
    padding: 10px 0;
    font-size: 15px;
    font-weight: bold;
    min-height: 20px;
}

QPushButton#primaryBtn:hover {
    background: #1565C0;
}

QPushButton#primaryBtn:pressed {
    background: #0D47A1;
}

QPushButton#primaryBtn:disabled {
    background: #BDBDBD;
}

/* ==== 顶部栏 ==== */
#userInfo {
    background: transparent;
    color: #FFFFFF;
    font-size: 15px;
}

#connectionStatus {
    background: transparent;
    color: #A5D6A7;
    font-size: 12px;
}

#logoutBtn {
    background: rgba(255,255,255,0.2);
    color: #FFFFFF;
    border: 1px solid rgba(255,255,255,0.3);
    border-radius: 4px;
    padding: 4px 16px;
    font-size: 12px;
}

#logoutBtn:hover {
    background: rgba(255,255,255,0.3);
}

/* ==== 左侧区域标题 ==== */
#sectionHeader {
    background: #EEEEEE;
    color: #616161;
    font-size: 12px;
    font-weight: bold;
    padding: 6px 12px;
    border-bottom: 1px solid #E0E0E0;
}

/* ==== 联系人列表 ==== */
QListWidget#contactList {
    background: #FFFFFF;
    border: none;
    outline: none;
    font-size: 13px;
}

QListWidget#contactList::item {
    padding: 8px 12px;
    border-bottom: 1px solid #F5F5F5;
    color: #424242;
}

QListWidget#contactList::item:hover {
    background: #E3F2FD;
}

QListWidget#contactList::item:selected {
    background: #BBDEFB;
    color: #1565C0;
}

/* ==== 小按钮 ==== */
QPushButton#smallBtn {
    background: transparent;
    color: #1976D2;
    border: 1px solid #BBDEFB;
    border-radius: 4px;
    padding: 4px 8px;
    font-size: 11px;
}

QPushButton#smallBtn:hover {
    background: #E3F2FD;
}

/* ==== 聊天标题 ==== */
#chatTitle {
    background: #FFFFFF;
    color: #424242;
    font-size: 15px;
    font-weight: bold;
    padding: 10px 12px;
    border-bottom: 1px solid #E0E0E0;
}

/* ==== 聊天显示区 ==== */
QTextBrowser#chatDisplay {
    background: #F5F7FA;
    border: none;
    padding: 4px;
    font-size: 13px;
}

/* ==== 聊天输入区 ==== */
QTextEdit#chatInput {
    border: 1px solid #E0E0E0;
    border-radius: 6px;
    padding: 8px 12px;
    background: #FFFFFF;
    color: #212121;
    font-size: 13px;
}

QTextEdit#chatInput:focus {
    border-color: #1976D2;
}

/* ==== 发送按钮 ==== */
QPushButton#sendBtn {
    background: #1976D2;
    color: #FFFFFF;
    border: none;
    border-radius: 6px;
    font-size: 14px;
    font-weight: bold;
}

QPushButton#sendBtn:hover {
    background: #1565C0;
}

QPushButton#sendBtn:pressed {
    background: #0D47A1;
}

QPushButton#sendBtn:disabled {
    background: #BDBDBD;
}

/* ==== QSplitter 手柄 ==== */
QSplitter#mainSplitter::handle {
    background: #E0E0E0;
}

/* ==== QMessageBox, QInputDialog ==== */
QMessageBox, QInputDialog {
    background: #FFFFFF;
}

QInputDialog QLineEdit {
    padding: 6px 10px;
    border: 1px solid #E0E0E0;
    border-radius: 4px;
    font-size: 13px;
}
)";

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setStyleSheet(kStyleQSS);

    LoginDialog loginDialog;

    // 未登录直接退出
    if (loginDialog.exec() != QDialog::Accepted) {
        return 0;
    }

    auto *window = new ChatWindow(loginDialog.socket(), loginDialog.loginResponse());
    window->setAttribute(Qt::WA_DeleteOnClose);

    // 注销 → 重新显示登录
    QObject::connect(window, &ChatWindow::loggedOut, [&]() {
        LoginDialog *ld = new LoginDialog;
        if (ld->exec() == QDialog::Accepted) {
            ChatWindow *cw = new ChatWindow(ld->socket(), ld->loginResponse());
            cw->setAttribute(Qt::WA_DeleteOnClose);
            QObject::connect(cw, &ChatWindow::loggedOut, [&]() {
                // 简单处理：递归嵌套一层即可
                LoginDialog *ld2 = new LoginDialog;
                if (ld2->exec() == QDialog::Accepted) {
                    ChatWindow *cw2 = new ChatWindow(ld2->socket(), ld2->loginResponse());
                    cw2->setAttribute(Qt::WA_DeleteOnClose);
                    cw2->show();
                }
            });
            cw->show();
        }
    });

    window->show();
    return app.exec();
}
