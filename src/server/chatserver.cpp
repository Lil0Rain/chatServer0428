#include "chatserver.hpp"
#include "chatservice.hpp"
#include "json.hpp"
#include <functional>
#include "muduo/base/Logging.h"
using json = nlohmann::json;

ChatServer::ChatServer(EventLoop *loop,
                       const InetAddress &listenAddr,
                       const string &nameArg) : _server(loop, listenAddr, nameArg), _loop(loop)
{
    _server.setConnectionCallback([this](const TcpConnectionPtr &conn)
                                  { this->onConnection(conn); });
    _server.setMessageCallback([this](const TcpConnectionPtr &conn,
                                      Buffer *buf,
                                      Timestamp time)
                               { this->onMessage(conn, buf, time); });
    _server.setThreadNum(4);
}

void ChatServer::onConnection(const TcpConnectionPtr &conn)
{
    if (conn->connected())
    {
        std::cout << conn->peerAddress().toIpPort() << " -> " << conn->localAddress().toIpPort() << " state:online" << std::endl;
    }
    else
    {
        std::cout << conn->peerAddress().toIpPort() << " -> " << conn->localAddress().toIpPort() << " state:offline" << std::endl;
        ChatService::instance()->clientCloseException(conn);
    }
}

void ChatServer::onMessage(const TcpConnectionPtr &conn,
                           Buffer *buf,
                           Timestamp time)
{
    try
    {
        string buff = buf->retrieveAllAsString();
        // 数据的反序列化
        json js = json::parse(buff);
        // 达到的目的：完全解耦网络模块代码与业务模块代码
        // 通过js["msgid"]获取消息id(绑定回调操作)，调用业务handler conn js time
        auto handler = ChatService::instance()->getHandler(js["msgid"].get<int>());
        // 回调消息绑定好的事件处理器，来执行相应的业务处理
        handler(conn, js, time);
    }
    catch (const std::exception &e)
    {
        LOG_ERROR << "onMessage exception: " << e.what();
    }
}

void ChatServer::start()
{
    _server.start();
}