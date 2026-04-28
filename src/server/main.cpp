#include <cstdint>

#include "chatserver.hpp"
#include "chatservice.hpp"
#include "muduo/base/Logging.h"
#include "signal.h"
using namespace std;

// 处理服务器ctrl+c结束后，重置user状态信息
void resetHandler(int) {
	ChatService::instance()->reset();
	exit(0);
}

int main(int argc, char **argv) {
	if (argc < 3) {
		cerr << "command invalid example: ./ChatServer 192.168.112.129 6000" << endl;
		exit(-1);
	}
	char *ip = argv[1];
	uint16_t port = atoi(argv[2]);

	signal(SIGINT, resetHandler);

	EventLoop loop;
	InetAddress addr(ip, port);
	ChatServer server(&loop, addr, "Chatserver");

	server.start();

	try {
		loop.loop();  // epool_wait以阻塞方式等待新用户连接，已连接用户的读写事件
	} catch (const std::exception &e) {
		LOG_ERROR << "server exit with exception: " << e.what();
		ChatService::instance()->reset();
	}
}