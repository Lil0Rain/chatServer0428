#pragma once
#ifndef USERMUDUO_HPP
#define USERMUDUO_HPP
#include "user.hpp"
#include "db.h"

// user表的数据操作类
class userMuduo
{
public:
    //user表的增加方法
    bool insert(User &user);
    //根据id查询用户信息
    User query(int id);
    //更新用户状态信息
    bool updateState(User &user);
    //重置用户的状态信息
    void resetState();

private:
};

#endif // USERMUDUO_HPP