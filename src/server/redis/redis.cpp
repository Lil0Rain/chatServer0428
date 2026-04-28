#include "redis.hpp"

#include <hiredis/hiredis.h>
#include <hiredis/read.h>

#include <iostream>
#include <ostream>
#include <string>
#include <thread>

// 这是一段话用来测试github推送功能是否正常
// 这是一段话用来测试github推送功能是否正常
// 这是一段话用来测试github推送功能是否正常

// 这是一段话用于测试命令行上传功能是否正常
// 这是一段话用于测试命令行上传功能是否正常
// 这是一段话用于测试命令行上传功能是否正常

Redis::Redis() : _publish_context(nullptr), _subscribe_context(nullptr) {
}

Redis::~Redis() {
	if (_publish_context != nullptr) {
		redisFree(_publish_context);
	}
	if (_subscribe_context != nullptr) {
		redisFree(_subscribe_context);
	}
}

bool Redis::connect() {
	// 负责publish发布消息的上下文连接
	_publish_context = redisConnect("192.168.112.129", 6379);
	if (nullptr == _publish_context) {
		std::cerr << "connect redis ERROR" << std::endl;
		return false;
	}
	_subscribe_context = redisConnect("192.168.112.129", 6379);
	if (nullptr == _subscribe_context) {
		std::cerr << "connect redis ERROR" << std::endl;
		return false;
	}
	// 在单独的线程中监听通道中的消息，有消息给业务层上报
	std::thread t([this]() {
		this->observer_channel_message();
	});
	t.detach();
	std::cout << "connect redis-service success!" << std::endl;
	return true;
}

bool Redis::publish(int channel, std::string message) {
	redisReply* reply =
		static_cast<redisReply*>(redisCommand(_publish_context, "PUBLISH %d %s", channel, message.c_str()));
	if (nullptr == reply) {
		std::cerr << "publish command failed" << std::endl;
		return false;
	}
	freeReplyObject(reply);
	return true;
}

// 向redis指定的通道订阅消息
bool Redis::subscribe(int channel) {
	// SUBCRIBE命令本身会造成线程阻塞等待通道里面发生消息，这里只做订阅通道，不接收通道消息
	// 通道消息的接收专门在observer_channel_message函数中的独立线程进行
	// 只负责发送命令，不阻塞接收redis service响应消息，否则和notiyfMsg线程抢占响应资源
	if (REDIS_ERR == redisAppendCommand(this->_subscribe_context, "SUBSCRIBE %d", channel)) {
		std::cerr << "subscribe command failed!" << std::endl;
		return false;
	}
	// redisBUfferWrite可以循环发送缓存区，直到缓存区数据发送完毕（done被置于1）
	int done = 0;
	while (!done) {
		if (REDIS_ERR == redisBufferWrite(this->_subscribe_context, &done)) {
			std::cerr << "subscribe command failed!" << std::endl;
			return false;
		}
	}
	return true;
}

// 向redis指定的通道取消订阅消息
bool Redis::unsubscribe(int channel) {
	if (REDIS_ERR == redisAppendCommand(this->_subscribe_context, "UNSUBSCRIBE %d", channel)) {
		std::cerr << "unsubscribe command failed!" << std::endl;
		return false;
	}
	// redisBUfferWrite可以循环发送缓存区，直到缓存区数据发送完毕（done被置于1）
	int done = 0;
	while (!done) {
		if (REDIS_ERR == redisBufferWrite(this->_subscribe_context, &done)) {
			std::cerr << "unsubscribe command failed!" << std::endl;
			return false;
		}
	}
	return true;
}

// 在独立线程中接受订阅通道中的消息
void Redis::observer_channel_message() {
	redisReply* reply = nullptr;
	while (REDIS_OK == redisGetReply(this->_subscribe_context, (void**)&reply)) {
		// 订阅收到的消息是一个带三个元素的数组
		if (reply != nullptr && reply->type == REDIS_REPLY_ARRAY && reply->elements == 3 && reply->element[0] != nullptr
			&& reply->element[1] != nullptr && reply->element[2] != nullptr && reply->element[0]->str != nullptr
			&& reply->element[1]->str != nullptr && reply->element[2]->str != nullptr
			&& std::string(reply->element[0]->str) == "message") {
			// 给业务层上报通道消息
			_notify_message_handler(atoi(reply->element[1]->str), reply->element[2]->str);
		}
		freeReplyObject(reply);
	}
	std::cerr << ">>>>>>>>>>> observer_channel_message quit <<<<<<<<<<<<" << std::endl;
}

// 初始化向业务层上报通道消息的回调对象
void Redis::init_notify_handler(std::function<void(int, std::string)> fn) {
	this->_notify_message_handler = fn;
}
