#pragma once
#ifndef OFFLINE_MSG_MUDUO_HPP
#define OFFLINE_MSG_MUDUO_HPP
#include <list>
#include <string>
#include <iostream>

//提供离线消息表的操作接口方法
class offlineMsgMuduo
{
    public:
        // 获取离线消息
        std::list<std::string> OfflineMsgQueue(int userid);
        // 添加离线消息
        bool addOfflineMsg(int userid, const std::string& msg);
        // 删除离线消息
        bool deleteOfflineMsg(int userid);
};



#endif