#include "db.h"

#include <muduo/base/Logging.h>

#include "Connection.h"
#include "mysql.h"

static std::string server = "127.0.0.1";
static std::string user = "root";
static std::string password = "123456";
static std::string dbname = "chat";

Mysql::Mysql() {
	// _conn = mysql_init(nullptr);  // 并非链接，仅仅提供了一块存储连接的资源空间
}

Mysql::~Mysql() {
	// if (_conn != nullptr) {
	// 	mysql_close(_conn);	 // 释放连接资源
	// }
}

// 连接数据库
bool Mysql::connect() {
	_conn = ConnectionPool::getConnectionPool()->getConnection();
	if (_conn != nullptr) {
		LOG_INFO << "Connect Mysql success!";
		return true;
	} else {
		LOG_INFO << "连接数据库失败!";
	}
	return false;
}

// 更新操作
bool Mysql::update(const std::string &sql) {
	if (!_conn->update(sql)) {
		LOG_INFO << __FILE__ << ":" << __LINE__ << ":" << sql << "更新失败!";
		return false;
	}
	return true;
}

// 查询操作
MYSQL_RES *Mysql::query(const std::string &sql) {
	MYSQL_RES *res = _conn->query(sql);
	if (res == nullptr) {
		LOG_INFO << __FILE__ << ":" << __LINE__ << ":" << sql << "查询失败!";
		return nullptr;
	}
	return res;
}

// 获取连接
MYSQL *Mysql::getConnection() {
	return _conn->getConnection();
}