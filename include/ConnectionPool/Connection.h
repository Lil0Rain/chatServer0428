#pragma once
#include <mysql.h>

#include <chrono>
#include <ctime>
#include <string>

using namespace std;

class Connection {
public:
	Connection();
	// �ͷ����ݿ�������Դ
	~Connection();
	// �������ݿ�
	bool connect(string ip, unsigned short port, string user, string password, string dbname);
	// ���²��� insert��delete��update
	bool update(const string& sql);
	MYSQL* getConnection();
	// ��ѯ���� select
	MYSQL_RES* query(const string& sql);

	// �������ӵ���ʼ�Ŀ���ʱ���
	void refresh_alivetime() {
		_alivetime = std::chrono::steady_clock::now();
	};

	std::chrono::milliseconds get_alivetime() {
		return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - _alivetime);
	};

private:
	MYSQL* _conn;									   // ��ʾ��MySQL Server��һ������
	std::chrono::steady_clock::time_point _alivetime;  // �������״̬��Ĵ��ʱ��
};
