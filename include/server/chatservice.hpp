#pragma once
#ifndef CHAT_SERVICE_HPP
#define CHAT_SERVICE_HPP

#include <functional>
#include <mutex>
#include <unordered_map>

#include "friendMuduo.hpp"
#include "groupMuduo.hpp"
#include "json.hpp"
#include "muduo/net/TcpConnection.h"
#include "offlineMsgMuduo.hpp"
#include "redis.hpp"
#include "userMuduo.hpp"
using namespace muduo;
using namespace muduo::net;
using namespace std;
using json = nlohmann::json;

// 表示处理消息的事件回调方法类型
using MsgHandler = function<void(const TcpConnectionPtr &conn, const json &js, Timestamp)>;	 // 消息处理器类型

// 聊天服务器业务类
class ChatService {
public:
	// 单例模式
	static ChatService *instance();
	// 客户端异常关闭
	void clientCloseException(const TcpConnectionPtr &conn);
	// 服务器整体退出时下线所有在线用户
	void reset();
	// 处理登陆业务
	void login(const TcpConnectionPtr &conn, const json &js, Timestamp time);
	// 处理注册业务
	void reg(const TcpConnectionPtr &conn, const json &js, Timestamp time);
	// 处理注销业务
	void offline(const TcpConnectionPtr &conn, const json &js, Timestamp time);
	// 一对一聊天业务
	void oneChat(const TcpConnectionPtr &conn, const json &js, Timestamp time);
	// 添加好友业务
	void addFriend(const TcpConnectionPtr &conn, const json &js, Timestamp time);
	// 创建群组业务
	void creatgroup(const TcpConnectionPtr &conn, const json &js, Timestamp time);
	// 加入群组业务
	void addgroup(const TcpConnectionPtr &conn, const json &js, Timestamp time);
	// 群聊天业务
	void chatgroup(const TcpConnectionPtr &conn, const json &js, Timestamp time);
	// 用户注销业务
	void logout(const TcpConnectionPtr &conn, const json &js, Timestamp time);
	// 获取消息对应的处理器
	MsgHandler getHandler(int msgid);
	// 处理redis订阅消息
	void handleRedisSubscribeMessage(int channel, std::string message);

private:
	// 采用单例模式；构造函数私有化
	ChatService();
	// 存储消息id和对应的业务处理方法
	unordered_map<int, MsgHandler> _msgHandlerMap;
	// 存储用户的通信连接（会在程序运行中更新，需要考虑线程安全）
	unordered_map<int, TcpConnectionPtr> _userConnMap;
	// 保证更新_userConnMap时的线程安全
	mutex _connMutex;

	// 数据操作类对象
	userMuduo _userMuduo;
	offlineMsgMuduo _offlineMsgMuduo;
	friendMuduo _friendMuduo;
	groupMuduo _groupMuduo;

	// redis对象
	Redis _redis;
};

#endif