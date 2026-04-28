#include "chatservice.hpp"

#include <mutex>
#include <string>

#include "muduo/base/Logging.h"
#include "offlineMsgMuduo.hpp"
#include "public.hpp"
#include "user.hpp"

// 获取单例对象的接口函数
ChatService *ChatService::instance() {
	static ChatService service;
	return &service;
}

// 客户端异常关闭
void ChatService::clientCloseException(const TcpConnectionPtr &conn) {
	User userChanged;
	bool found = false;
	{
		lock_guard<mutex> lock(_connMutex);
		for (auto it = _userConnMap.begin(); it != _userConnMap.end(); ++it) {
			if (it->second == conn) {
				userChanged.setId(it->first);
				userChanged.setState("offline");
				_userConnMap.erase(it);
				found = true;
				break;
			}
		}
	}

	_redis.unsubscribe(userChanged.getId());

	// 这里还需要更新用户的状态信息，设置为offline
	if (found) {
		_userMuduo.updateState(userChanged);
	}
}

// 服务器退出时下线所有在线用户
void ChatService::reset() {
	// 将onlin状态的用户重置weioffline
	_userMuduo.resetState();
}

// 注册消息以及对应的Handler回调操作
ChatService::ChatService() {
	// 注册消息以及对应的Handler回调操作
	_msgHandlerMap.insert({LOGIN_MSG, [this](const TcpConnectionPtr &conn, const json &js, Timestamp time) {
							   this->login(conn, js, time);
						   }});
	_msgHandlerMap.insert({REG_MSG, [this](const TcpConnectionPtr &conn, const json &js, Timestamp time) {
							   this->reg(conn, js, time);
						   }});
	_msgHandlerMap.insert({OFFLINE_MSG, [this](const TcpConnectionPtr &conn, const json &js, Timestamp time) {
							   this->offline(conn, js, time);
						   }});
	_msgHandlerMap.insert({ONE_CHAT_MSG, [this](const TcpConnectionPtr &conn, const json &js, Timestamp time) {
							   this->oneChat(conn, js, time);
						   }});
	_msgHandlerMap.insert({ADD_FRIEND_MSG, [this](const TcpConnectionPtr &conn, const json &js, Timestamp time) {
							   this->addFriend(conn, js, time);
						   }});
	_msgHandlerMap.insert({CREATE_GROUP_MSG, [this](const TcpConnectionPtr &conn, const json &js, Timestamp time) {
							   this->creatgroup(conn, js, time);
						   }});
	_msgHandlerMap.insert({ADD_GROUP_MSG, [this](const TcpConnectionPtr &conn, const json &js, Timestamp time) {
							   this->addgroup(conn, js, time);
						   }});
	_msgHandlerMap.insert({GROUP_CHAT_MSG, [this](const TcpConnectionPtr &conn, const json &js, Timestamp time) {
							   this->chatgroup(conn, js, time);
						   }});
	_msgHandlerMap.insert({LOGOUT_MSG, [this](const TcpConnectionPtr &conn, const json &js, Timestamp time) {
							   this->logout(conn, js, time);
						   }});

	// 设置上报消息的回调
	_redis.init_notify_handler([this](int channel, std::string message) {
		this->handleRedisSubscribeMessage(channel, message);
	});

	if (_redis.connect()) {
		// 连接redis服务器
	}
}

MsgHandler ChatService::getHandler(int msgid) {
	// 记录错误日志，msgid没有对应的Handler处理回调函数
	auto it = _msgHandlerMap.find(msgid);
	if (it == _msgHandlerMap.end()) {
		// 返回一个默认的处理器，空操作
		//[=]表示获取当前函数作用域的所有变量，值传递方式
		return [=](const TcpConnectionPtr &conn, const json &js, Timestamp time) {
			LOG_ERROR << "msgid: " << msgid << " can not find handler!";
		};
	} else {
		return _msgHandlerMap[msgid];
	}
}

// ORM框架 业务层操作的都是的对象 数据层具体的有数据库操作
// 处理登陆业务 id password
void ChatService::login(const TcpConnectionPtr &conn, const json &js, Timestamp time) {
	int id = js["id"].get<int>();
	string password = js["password"];

	User user = _userMuduo.query(id);
	if (user.getId() == id && user.getPassword() == password) {
		if (user.getState() == "online") {
			// 登录失败
			json response;
			response["msgid"] = LOGIN_MSG_ACK;
			response["errno"] = 2;	// 2表示已在线
			response["errmsg"] = "账号已在线!";
			conn->send(response.dump());
			return;
		}

		// 登录成功 更新用户状态信息
		user.setState("online");
		// mysql的并发操作是由数据库本身来保证的，所以这里不需要考虑线程安全问题
		_userMuduo.updateState(user);
		// 登陆成功记录用户连接信息
		{
			lock_guard<mutex> lock(_connMutex);
			_userConnMap.insert({id, conn});
		}

		// 订阅用户id的消息
		_redis.subscribe(id);

		json response;
		response["msgid"] = LOGIN_MSG_ACK;
		response["errno"] = 0;	// 0表示没有错误
		response["id"] = user.getId();
		response["name"] = user.getName();

		// 登录成功检查用户是否存在离线消息
		std::list<std::string> offlineMsgList = _offlineMsgMuduo.OfflineMsgQueue(id);
		if (!offlineMsgList.empty()) {
			response["offlinemsg"] = offlineMsgList;
			// 将离线消息从数据库中删除
			_offlineMsgMuduo.deleteOfflineMsg(id);
		}

		// 登陆成功查询用户的好友信息
		vector<User> friendVec = _friendMuduo.query(id);
		if (!friendVec.empty()) {
			vector<string> friendVecStr;
			for (const User &friendUser : friendVec) {
				json js;
				js["id"] = friendUser.getId();
				js["name"] = friendUser.getName();
				js["state"] = friendUser.getState();
				friendVecStr.push_back(js.dump());
			}
			response["friend"] = friendVecStr;
		}

		// 登陆成功查询用户的群组信息
		vector<Group> groupuserVec = _groupMuduo.queryGroups(id);
		if (!groupuserVec.empty()) {
			vector<string> groupv;
			for (Group &group : groupuserVec) {
				json groupjs;
				groupjs["id"] = group.getId();
				groupjs["groupname"] = group.getName();
				groupjs["groupdesc"] = group.getDesc();
				vector<string> userV;
				for (GroupUser &user : group.getUsers()) {
					json userjs;
					userjs["id"] = user.getId();
					userjs["name"] = user.getName();
					userjs["state"] = user.getState();
					userjs["role"] = user.getRole();
					userV.push_back(userjs.dump());
				}
				groupjs["users"] = userV;
				groupv.push_back(groupjs.dump());
			}
			response["groups"] = groupv;
		}

		conn->send(response.dump());
	} else {
		// 登录失败
		json response;
		response["msgid"] = LOGIN_MSG_ACK;
		response["errno"] = 1;	// 1表示有错误
		response["errmsg"] = "账号或密码错误!";
		conn->send(response.dump());
	}
}

// 处理注册业务
void ChatService::reg(const TcpConnectionPtr &conn, const json &js, Timestamp time) {
	string name = js["name"];
	string password = js["password"];
	User user;
	user.setName(name);
	user.setPassword(password);
	bool state = _userMuduo.insert(user);
	if (state) {
		// 注册成功，向客户端返回注册成功的响应信息
		json response;
		response["msgid"] = REG_MSG_ACK;
		response["errno"] = 0;	// 0表示没有错误
		response["id"] = user.getId();
		conn->send(response.dump());
	} else {
		// 注册失败，向客户端返回注册失败的响应信息
		json response;
		response["msgid"] = REG_MSG_ACK;
		response["errno"] = 1;	// 1表示有错误
		conn->send(response.dump());
	}
}

// 处理注销业务
void ChatService::offline(const TcpConnectionPtr &conn, const json &js, Timestamp time) {
	int id = js["id"].get<int>();
	User user = _userMuduo.query(id);
	if (user.getState() == "online") {

		// 更新用户状态信息
		user.setState("offline");
		_userMuduo.updateState(user);
		// 注销成功删除用户连接信息
		{
			lock_guard<mutex> lock(_connMutex);
			_userConnMap.erase(id);
		}

		json response;
		response["msgid"] = OFFLINE_MSG_ACK;
		response["errno"] = 0;	// 0表示没有错误
		conn->send(response.dump());
	} else {
		LOG_ERROR << "User with id: " << id << " is not online!";

		json response;
		response["msgid"] = OFFLINE_MSG_ACK;
		response["errno"] = 1;	// 1表示有错误
		response["errmsg"] = "用户未在线!";
		conn->send(response.dump());
	}
}

// 处理一对一聊天业务
void ChatService::oneChat(const TcpConnectionPtr &conn, const json &js, Timestamp time) {
	// 获取发送目标用户id
	int toid = js["toid"].get<int>();
	// 访问连接信息表
	{
		lock_guard<mutex> lock(_connMutex);
		auto it = _userConnMap.find(toid);
		if (it != _userConnMap.end()) {
			// 在线，转发消息 服务器主动推送消息给toid用户
			it->second->send(js.dump());
			return;
		}
	}
	User user = _userMuduo.query(toid);
	if (user.getState() == "online") {
		_redis.publish(toid, js.dump());
		return;
	}
	// 不在线，存储离线消息表
	_offlineMsgMuduo.addOfflineMsg(toid, js.dump());
}

// 添加好友业务
void ChatService::addFriend(const TcpConnectionPtr &conn, const json &js, Timestamp time) {
	int userid = js["id"].get<int>();
	int friid = js["friendid"].get<int>();

	_friendMuduo.addFriend(userid, friid);
}

// 创建群组业务
void ChatService::creatgroup(const TcpConnectionPtr &conn, const json &js, Timestamp time) {
	int userid = js["id"].get<int>();
	string name = js["groupname"];
	string desc = js["groupdesc"];

	Group group(-1, name, desc);
	if (_groupMuduo.creatGroup(group)) {
		_groupMuduo.addGroup(userid, group.getId(), "creator");
	}
}

// 加入群组业务
void ChatService::addgroup(const TcpConnectionPtr &conn, const json &js, Timestamp time) {
	int userid = js["id"].get<int>();
	int groupid = js["groupid"].get<int>();
	string role = "normal";
	_groupMuduo.addGroup(userid, groupid, role);
}

// 群聊天业务
void ChatService::chatgroup(const TcpConnectionPtr &conn, const json &js, Timestamp time) {
	int userid = js["id"].get<int>();
	int groupid = js["groupid"].get<int>();
	vector<int> useridVec = _groupMuduo.queryGroupUsers(userid, groupid);
	{
		lock_guard<mutex> lock(_connMutex);
		for (int id : useridVec) {
			auto it = _userConnMap.find(id);
			if (it != _userConnMap.end()) {
				it->second->send(js.dump());
			} else {
				User user = _userMuduo.query(id);
				if (user.getState() == "online") {
					_redis.publish(user.getId(), js.dump());
				} else {
					_offlineMsgMuduo.addOfflineMsg(id, js.dump());
				}
			}
		}
	}
}

// 用户注销业务
void ChatService::logout(const TcpConnectionPtr &conn, const json &js, Timestamp time) {
	int id = js["id"].get<int>();
	User user = _userMuduo.query(id);
	if (user.getState() == "online") {

		// 更新用户状态信息
		user.setState("offline");
		_userMuduo.updateState(user);
		// 注销成功删除用户连接信息
		{
			lock_guard<mutex> lock(_connMutex);
			_userConnMap.erase(id);
		}
		_redis.unsubscribe(id);	 // 注销成功取消订阅用户id的消息

		json response;
		response["msgid"] = LOGOUT_MSG_ACK;
		response["errno"] = 0;	// 0表示没有错误
		conn->send(response.dump());
	} else {
		LOG_ERROR << "User with id: " << id << " is not online!";

		json response;
		response["msgid"] = LOGOUT_MSG_ACK;
		response["errno"] = 1;	// 1表示有错误
		response["errmsg"] = "用户未在线!";
		conn->send(response.dump());
	}
}

// 处理redis订阅消息
void ChatService::handleRedisSubscribeMessage(int channel, std::string message) {
	json js = json::parse(message.c_str());

	lock_guard<mutex> lock(_connMutex);
	auto it = _userConnMap.find(channel);
	if (it != _userConnMap.end()) {
		it->second->send(js.dump());
		return;
	}
	_offlineMsgMuduo.addOfflineMsg(channel, message);
}