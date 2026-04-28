#include <iostream>
#include "userMuduo.hpp"
#include "db.h"

bool userMuduo::insert(User &user)
{
    // 1. 连接数据库
    Mysql mysql;
    if (!mysql.connect())
    {
        cerr << "Failed to connect to database" << endl;
        return false;
    }

    // 2. 构造SQL语句
    char sql[1024] = {0};
    sprintf(sql, "INSERT INTO user(name, password, state) VALUES('%s', '%s', '%s')",
            user.getName().c_str(), user.getPassword().c_str(), user.getState().c_str());

    // 3. 执行SQL语句
    if (mysql.update(sql))
    {
        //获取插入成功的用户数据生成的主键id
        user.setId(mysql_insert_id(mysql.getConnection()));
        return true;
    }
    else
    {
        cerr << "Failed to insert user into database" << endl;
        return false;
    }
}

User userMuduo::query(int id)
{
    // 1. 连接数据库
    Mysql mysql;
    if (!mysql.connect())
    {
        cerr << "Failed to connect to database" << endl;
        return User();
    }

    // 2. 构造SQL语句
    char sql[1024] = {0};
    sprintf(sql, "SELECT * FROM user WHERE id=%d", id);

    // 3. 执行SQL语句
    MYSQL_RES *res = mysql.query(sql);
    if(res!=nullptr)
    {
        //将查询结果的第一行数据封装成一个user对象返回
        MYSQL_ROW row = mysql_fetch_row(res);
        if(row!=nullptr)
        {
            User user;
            user.setId(atoi(row[0]));
            user.setName(row[1]);
            user.setPassword(row[2]);
            user.setState(row[3]);
            mysql_free_result(res);//释放资源
            return user;
        }
        else
        {
            mysql_free_result(res);
            return User();
        }
    }
    else
    {
        cerr << "Failed to query user from database" << endl;
        return User();
    }
    
}

bool userMuduo::updateState(User &user)
{
    // 1. 连接数据库
    Mysql mysql;
    if (!mysql.connect())
    {
        cerr << "Failed to connect to database" << endl;
        return false;
    }

    // 2. 构造SQL语句
    char sql[1024] = {0};
    sprintf(sql, "UPDATE user SET state='%s' WHERE id=%d",
            user.getState().c_str(), user.getId());

    // 3. 执行SQL语句
    if (mysql.update(sql))
    {
        return true;
    }
    else
    {
        cerr << "Failed to update user in database" << endl;
        return false;
    }

    
}

void userMuduo::resetState()
{
    // 1. 连接数据库
    Mysql mysql;
    if (!mysql.connect())
    {
        cerr << "Failed to connect to database" << endl;
    }
    //2. 组装sql语句
    char sql[1024] ={"UPDATE user SET state = 'offline' WHERE state = 'online'"};
    //3. 执行sql语句
    mysql.update(sql);
}