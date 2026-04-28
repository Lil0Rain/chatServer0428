#include "offlineMsgMuduo.hpp"
#include "db.h"

// 获取离线消息
std::list<std::string> offlineMsgMuduo::OfflineMsgQueue(int userid)
{
    std::list<std::string> msgList;
    Mysql mysql;
    if (!mysql.connect())
    {
        std::cerr << "Failed to connect to database" << std::endl;
        return msgList;
    }
    char sql[1024] = {0};
    sprintf(sql, "SELECT message FROM OfflineMessage WHERE userid=%d", userid);
    MYSQL_RES *res = mysql.query(sql);
    if (res != nullptr)
    {
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res)) != nullptr)
        {
            msgList.push_back(row[0]);
        }
        mysql_free_result(res);
        return msgList;
    }
    else
    {
        std::cerr << "Failed to query offline messages from database" << std::endl;
        return msgList;
    }
}
// 添加离线消息
bool offlineMsgMuduo::addOfflineMsg(int userid, const std::string &msg)
{
    Mysql mysql;
    if (!mysql.connect())
    {
        std::cerr << "Failed to connect to database" << std::endl;
        return false;
    }
    // 2. 构造SQL语句
    char sql[1024] = {0};
    sprintf(sql, "INSERT INTO OfflineMessage(userid, message) VALUES(%d, '%s')",
            userid, msg.c_str());

    // 3. 执行SQL语句
    if (mysql.update(sql))
    {
        return true;
    }
    else
    {
        std::cerr << "Failed to insert user into database" << std::endl;
        return false;
    }
}
// 删除离线消息
bool offlineMsgMuduo::deleteOfflineMsg(int userid)
{
    Mysql mysql;
    if (!mysql.connect())
    {
        std::cerr << "Failed to connect to database" << std::endl;
        return false;
    }
    // 2. 构造SQL语句
    char sql[1024] = {0};
    sprintf(sql, "DELETE FROM OfflineMessage WHERE userid = %d", userid);

    // 3. 执行SQL语句
    if (mysql.update(sql))
    {
        return true;
    }
    else
    {
        std::cerr << "Failed to delete offline messages" << std::endl;
        return false;
    }
}