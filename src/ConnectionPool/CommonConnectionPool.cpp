// CommonConnectionPool.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#include <iostream>
#define _CRT_SECURE_NO_WARNINGS
#include <thread>

#include "CommonConnectionPool.h"
#include "public.hpp"

// 获取实例
ConnectionPool* ConnectionPool::getConnectionPool() {
	static ConnectionPool pool;	 // 懒汉式单例模式，第一次调用时创建对象实例，之后调用直接返回对象实例
	return &pool;				 // 返回对象实例地址
}

// 加载配置文件，初始化连接池参数
namespace {
string trim(const string& s) {
	size_t begin = s.find_first_not_of(" \t\r\n");
	if (begin == string::npos) {
		return "";
	}
	size_t end = s.find_last_not_of(" \t\r\n");
	return s.substr(begin, end - begin + 1);
}
}  // namespace

bool ConnectionPool::loadConfigFile() {
	FILE* pf = fopen("/home/userme/ChatServer/chatServer0428/mysql.ini", "r");
	if (pf == nullptr) {
		LOG("mysql.ini配置文件打开失败");
		return false;
	}

	char line[1024] = {0};
	while (fgets(line, sizeof(line), pf) != nullptr) {
		string str = trim(line);

		if (str.empty()) {
			continue;
		}
		if (str[0] == '#' || str[0] == ';') {
			continue;
		}

		size_t idx = str.find('=');
		if (idx == string::npos) {
			continue;
		}

		string key = trim(str.substr(0, idx));
		string value = trim(str.substr(idx + 1));

		if (key.empty() || value.empty()) {
			continue;
		}

		if (key == "ip") {
			_ip = value;
		} else if (key == "port") {
			_port = static_cast<unsigned short>(stoi(value));
		} else if (key == "dbname") {
			_dbname = value;
		} else if (key == "username") {
			_username = value;
		} else if (key == "password") {
			_password = value;
		} else if (key == "initialSize") {
			_initialSize = stoi(value);
		} else if (key == "maxSize") {
			_maxSize = stoi(value);
		} else if (key == "maxIdleTime") {
			_MaxIdleTime = stoi(value);
		} else if (key == "connectionTimeout") {
			_connectionTimeout = stoi(value);
		}
	}

	fclose(pf);

	if (_ip.empty() || _username.empty() || _dbname.empty() || _port == 0 || _initialSize <= 0
		|| _maxSize < _initialSize || _MaxIdleTime <= 0 || _connectionTimeout <= 0) {
		LOG("mysql.ini配置项不完整或不合法");
		return false;
	}

	return true;
}

// 构造函数
ConnectionPool::ConnectionPool() {
	if (!loadConfigFile()) {
		LOG("加载配置文件失败");
		return;
	}
	// 创建初始数量的链接
	for (int i = 0; i < _initialSize; i++) {
		Connection* p = new Connection();
		if (p->connect(_ip, _port, _username, _password, _dbname)) {
			p->refresh_alivetime();
			_connectionQue.push(p);
			_connectionCnt++;
		} else {
			cout << "Connect ERROR" << endl;
			delete p;
		}
	}
	// 一个可用连接都没有，视为连接池初始化失败
	if (_connectionCnt == 0) {
		LOG("init ERROR no DataBase Connect");
		return;
	}
	// 启动一个新的线程，作为连接池的生产者，专门负责生产新的数据库连接
	thread produce([this]() {
		this->produceConnectionTask();
	});
	produce.detach();
	/*thread produce(std::bind(&ConnectionPool::produceConnectionTask,this));*/
	// 启动一个新的线程，扫描多余的空闲连接，超过最大空闲时间的空闲连接进行多余的链接回收
	thread scanner([this]() {
		this->scannerConnectionTask();
	});
	scanner.detach();
}

ConnectionPool::~ConnectionPool() {
}

// 生产者线程实现
void ConnectionPool::produceConnectionTask() {
	for (;;) {
		unique_lock<mutex> lock(_queueMutex);
		// std::unique_lock<std::mutex> lock(_queueMutex);
		while (!_connectionQue.empty() || _connectionCnt >= _maxSize) {
			cv.wait(lock);	// 队列不空生产线程进入等待状态
		}
		lock.unlock();
		Connection* p = new Connection();
		if (p->connect(_ip, _port, _username, _password, _dbname)) {
			lock.lock();
			p->refresh_alivetime();
			_connectionQue.push(p);
			_connectionCnt++;
			cv.notify_all();
		} else {
			lock.lock();
			delete p;
		}
		// 连接数量没有到达上限，继续创建新连接

		// 通知消费者线程可以消费链接
		// cv.notify_all();
	}
}

// 给外部提供接口，从连接池中获取一个可用的链接
shared_ptr<Connection> ConnectionPool::getConnection() {
	unique_lock<mutex> lock(_queueMutex);
	bool wait_success = cv.wait_for(lock, chrono::milliseconds(_connectionTimeout), [this]() {
		return !_connectionQue.empty();
	});
	if (!wait_success) {
		LOG("获取空闲连接超时，获取链接失败！");
		return nullptr;
	}
	/*
		shared_ptr智能指针析构时，会将connection资源delete，需要自定义shared_ptr释放资源的方式，将connection对象直接归还予队列当中
	*/
	shared_ptr<Connection> sp(_connectionQue.front(), [this](Connection* pcon) {
		unique_lock<mutex> lock(_queueMutex);
		pcon->refresh_alivetime();
		_connectionQue.push(pcon);
		cv.notify_all();
	});
	_connectionQue.pop();
	if (_connectionQue.empty()) {
		cv.notify_all();  // 谁消费了队列中的最后一个，谁通知生产者生产
	}
	return sp;
}

// 扫描线程函数实现
void ConnectionPool::scannerConnectionTask() {
	for (;;) {
		// 通过sleep模拟定时效果
		this_thread::sleep_for(chrono::seconds(_MaxIdleTime));
		// 扫描整个队列，释放多余链接
		unique_lock<mutex> lock(_queueMutex);
		while (_connectionCnt > _initialSize && !_connectionQue.empty()) {
			Connection* p = _connectionQue.front();
			if (p->get_alivetime() > chrono::milliseconds(_MaxIdleTime * 1000)) {
				_connectionQue.pop();
				_connectionCnt--;
				delete p;  // 调用Connection的析构函数释放连接
			} else {
				break;	// 队首链接没有超过最大口空闲时间，直接跳过
			}
		}
	}
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门使用技巧:
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件