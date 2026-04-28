#pragma once
#ifndef CHATSERVER_PUBLIC_HPP
#define CHATSERVER_PUBLIC_HPP

/*
	server与client公共的头文件
*/
enum EnMsgType {
	LOGIN_MSG = 1,	   // 登录消息1
	LOGIN_MSG_ACK,	   // 登录响应消息2
	OFFLINE_MSG,	   // 注销消息3
	OFFLINE_MSG_ACK,   // 注销响应消息4
	REG_MSG,		   // 注册消息5
	REG_MSG_ACK,	   // 注册响应消息6
	ONE_CHAT_MSG,	   // 单聊消息7
	ADD_FRIEND_MSG,	   // 添加好友消息8
	CREATE_GROUP_MSG,  // 创建群组消息9
	ADD_GROUP_MSG,	   // 加入群组消息10
	GROUP_CHAT_MSG,	   // 群聊消息11
	LOGOUT_MSG,		   // 用户注销消息12
	LOGOUT_MSG_ACK
};

#endif