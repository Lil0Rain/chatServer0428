#pragma once
#ifndef FRIENDMUDUO_HPP
#define FRIENDMUDUO_HPP
#include "user.hpp"
#include <vector>


//维护好友信息的操作接口方法
class friendMuduo
{
public:
    //添加好友信息
    void addFriend(int userid, int friendid);
    //返回用户好友列表
    std::vector<User> query(int userid);
};



#endif