#include "Connection.h"

#include "public.hpp"

// ��ʼ�����ݿ�����
Connection::Connection() {
	_conn = mysql_init(nullptr);
}

// �ͷ����ݿ�������Դ
Connection::~Connection() {
	if (_conn != nullptr)
		mysql_close(_conn);
}

// �������ݿ�
/*
	IP��ַ �˿ں� �û��� ���� ���ݿ�����
*/
bool Connection::connect(string ip, unsigned short port, string user, string password, string dbname) {
	MYSQL* p = mysql_real_connect(_conn, ip.c_str(), user.c_str(), password.c_str(), dbname.c_str(), port, nullptr, 0);
	if (p != nullptr) {
		mysql_query(_conn, "set names gbk");
		return true;
	} else {
		LOG("数据库连接失败 ip=" + ip + " port=" + to_string(port) + " dbname=" + dbname
			+ " error=" + mysql_error(_conn));
		return false;
	}
}

// ���²��� insert��delete��update
bool Connection::update(const string& sql) {
	if (mysql_query(_conn, sql.c_str())) {
		LOG("更新失败:" + sql);
		return false;
	}
	return true;
}

MYSQL* Connection::getConnection() {
	return _conn;
}

// ��ѯ���� select
MYSQL_RES* Connection::query(const string& sql) {
	if (mysql_query(_conn, sql.c_str())) {
		LOG("查询失败:" + sql);
		return nullptr;
	}
	return mysql_store_result(_conn);
}