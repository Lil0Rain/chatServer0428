#pragma once
#ifndef DB_H
#define DB_H
#endif
#include <CommonConnectionPool.h>
#include <Connection.h>
#include <mysql/mysql.h>

#include <memory>
#include <string>

class Mysql {
public:
	Mysql();
	~Mysql();

	// 连接数据库
	bool connect();
	// 更新操作
	bool update(const std::string& sql);
	// 查询操作
	MYSQL_RES* query(const std::string& sql);
	// 获取连接
	MYSQL* getConnection();

private:
	// MYSQL *_conn;
	std::shared_ptr<Connection> _conn;
};
