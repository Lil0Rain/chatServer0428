#pragma once
#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <string>

#include "Connection.h"
using namespace std;

// ʵ�����ӳع���ģ��

class ConnectionPool {
public:
	static ConnectionPool* getConnectionPool();	 // ����ģʽ����ȡ���ӳض���ʵ��
	shared_ptr<Connection>
	getConnection();  // ��ȡ���ӳ��е�һ�����õĿ������ӣ�ʹ������ָ���Զ�����������������д��������ʹ�䲻ɾ�����ӣ����ǹ黹���ӣ�

private:
	ConnectionPool();	// ����ģʽ��˽�л����캯��
	~ConnectionPool();	// ����ģʽ��˽�л���������

	bool loadConfigFile();	// ���������ļ�����ʼ�����ӳز���

	void produceConnectionTask();  // �����ڶ����߳��У���������������

	void scannerConnectionTask();  // ɨ�����Ŀ������ӣ�����������ʱ��Ŀ������ӽ��ж�������ӻ���

	string _ip;				 // ���ݿ�IP��ַ
	unsigned short _port;	 // ���ݿ�˿ں�
	string _username;		 // ���ݿ��û���
	string _password;		 // ���ݿ�����
	string _dbname;			 // ���ݿ�����
	int _initialSize;		 // ���ӳس�ʼ������
	int _maxSize;			 // ���ӳ����������
	int _MaxIdleTime;		 // ���ӳ�������ʱ��
	int _connectionTimeout;	 // ���ӳ����ӳ�ʱʱ��

	queue<Connection*> _connectionQue;	// ���ӳض���
	mutex _queueMutex;					// ά�����ӳض����̰߳�ȫ�Ļ�����

	atomic_int _connectionCnt{0};  // ���ӳص�ǰ��������,������_maxSize

	condition_variable cv;
};
