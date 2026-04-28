#pragma once
#ifndef DB_H
#define DB_H
#endif
#include <mysql/mysql.h>
#include <string>

class Mysql
{
public:
    Mysql();
    ~Mysql();

    // 连接数据库
    bool connect();
    // 更新操作
    bool update(std::string sql);
    // 查询操作
    MYSQL_RES *query(std::string sql);
    // 获取连接
    MYSQL *getConnection();

private:
    MYSQL *_conn;
};
