#include "db.h"
#include <muduo/base/Logging.h>

static std::string server = "127.0.0.1";
static std::string user = "root";
static std::string password = "123456";
static std::string dbname = "chat";

Mysql::Mysql()
{
    _conn = mysql_init(nullptr); // 并非链接，仅仅提供了一块存储连接的资源空间
}
Mysql::~Mysql()
{
    if (_conn != nullptr)
    {
        mysql_close(_conn); // 释放连接资源
    }
}

// 连接数据库
bool Mysql::connect()
{
    MYSQL *p = mysql_real_connect(_conn, server.c_str(), user.c_str(),
                                  password.c_str(), dbname.c_str(), 3306, nullptr, 0);
    if (p != nullptr)
    {
        mysql_query(_conn, "set names gbk");
        LOG_INFO << "Connect Mysql success!";
    }
    else
    {
        LOG_INFO << "连接数据库失败!";
    }
    return p;
}
// 更新操作
bool Mysql::update(std::string sql)
{
    if (mysql_query(_conn, sql.c_str()))
    {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":"
                 << sql << "更新失败!";
        return false;
    }
    return true;
}
// 查询操作
MYSQL_RES *Mysql::query(std::string sql)
{
    if (mysql_query(_conn, sql.c_str()))
    {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":"
                 << sql << "查询失败!";
        return nullptr;
    }
    return mysql_use_result(_conn);
}

// 获取连接
MYSQL *Mysql::getConnection()
{
    return _conn;
}