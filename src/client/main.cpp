#include <semaphore.h>

#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "groupuser.hpp"
#include "json.hpp"

using json = nlohmann::json;

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "group.hpp"
#include "public.hpp"
#include "user.hpp"

// 记录当前系统登陆的用户信息
User g_currentUser;
// 记录当前登录用户的好友列表信息
std::vector<User> g_currentUserFriendList;
// 记录当前登录用户的群组列表信息
std::vector<Group> g_currentUserGroupList;
// 显示当前登录用户的基本信息
void showcurrentUserData();
// 控制主菜单页面程序
bool isMainMenuRunning = false;
// 记录登录是否成功
bool g_isLoginSuccess = false;
// 用于主线程等待接收线程处理登录/注册响应
sem_t rwsem;

// 接收线程
void readTaskHandler(int clientfd);
// 获取系统时间
string getcurrentTime();
// 主聊天页面程序
void mainMenu(int clientfd);

// 聊天客户端实现，main用于发送线程，子线程作为接收线程
int main(int argc, char **argv) {
	if (argc < 3) {
		cerr << "Command invalid example: ./ChatClient 192.168.112.129 6000" << endl;
		exit(-1);
	}

	// 解析通过命令行参数传递的ip与port
	char *ip = argv[1];
	uint16_t port = atoi(argv[2]);

	// 创建client端的socket
	int clientfd = socket(AF_INET, SOCK_STREAM, 0);
	if (-1 == clientfd) {
		cerr << "Socket create ERROR" << endl;
		exit(-1);
	}
	// 填写client所需要连接的server信息ip+port
	sockaddr_in server;
	std::memset(&server, 0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	server.sin_addr.s_addr = inet_addr(ip);

	// client与server进行连接
	if (-1 == connect(clientfd, (sockaddr *)&server, sizeof(sockaddr_in))) {
		cerr << "Connect Server ERROR" << endl;

		close(clientfd);
		exit(-1);
	}

	sem_init(&rwsem, 0, 0);
	std::thread readTask(readTaskHandler, clientfd);
	readTask.detach();

	for (;;) {
		// 显示首页面菜单 登录、注册、退出
		cout << "==========================" << endl;
		cout << " 1. Login " << endl;
		cout << " 2. Register " << endl;
		cout << " 3. Quit " << endl;
		cout << "==========================" << endl;
		cout << " Choice: " << endl;
		int choice = 0;
		cin >> choice;
		cin.get();	// 读取缓冲区残留的回车

		switch (choice) {
			case 1:	 // 登录业务
			{
				int id = 0;
				char password[50] = {0};
				cout << " userid " << endl;
				cin >> id;
				cin.get();
				cout << " password " << endl;
				cin.getline(password, 50);

				json js;
				js["msgid"] = LOGIN_MSG;
				js["id"] = id;
				js["password"] = password;
				string request = js.dump();

				int len = send(clientfd, request.c_str(), request.size(), 0);
				if (len == -1) {
					cerr << " Send Message ERROR! " << endl;
				} else {
					sem_wait(&rwsem);
					if (g_isLoginSuccess) {
						isMainMenuRunning = true;
						mainMenu(clientfd);
					}
				}
			} break;
			case 2: {
				char name[50] = {0};
				char password[50] = {0};
				cout << " username " << endl;
				cin.getline(name, 50);
				cout << " password " << endl;
				cin.getline(password, 50);

				json js;
				js["msgid"] = REG_MSG;
				js["name"] = name;
				js["password"] = password;
				string request = js.dump();

				int len = send(clientfd, request.c_str(), request.size(), 0);
				if (len == -1) {
					cerr << " Send Message ERROR! " << endl;
				} else {
					sem_wait(&rwsem);
				}
			} break;
			case 3: {
				exit(0);
			}
			default: {
				cerr << " Invalid input " << endl;
				break;
			}
		}
	}
}

void readTaskHandler(int clientfd) {
	for (;;) {
		char buffer[1024] = {0};
		int len = recv(clientfd, buffer, 1024, 0);
		if (-1 == len) {
			cerr << "recv error" << endl;
			close(clientfd);
			exit(-1);
		}

		if (0 == len) {
			cerr << "server close connection" << endl;
			close(clientfd);
			exit(-1);
		}
		// 接收ChatServer转发的数据，生成json数据对象
		json js = json::parse(buffer);
		int messagetype = js["msgid"].get<int>();

		if (LOGIN_MSG_ACK == messagetype) {
			if (0 != js["errno"].get<int>()) {
				cerr << js["errmsg"] << endl;
				g_isLoginSuccess = false;
			} else {
				g_isLoginSuccess = true;
				g_currentUserFriendList.clear();
				g_currentUserGroupList.clear();

				// 登陆成功，记录当前用户的id与name
				g_currentUser.setId(js["id"].get<int>());
				g_currentUser.setName(js["name"]);

				// 记录当前用户的好友列表信息
				if (js.contains("friend")) {
					vector<string> vec = js["friend"];
					for (string &str : vec) {
						json js = json::parse(str);
						User user;
						user.setId(js["id"]);
						user.setName(js["name"]);
						user.setState(js["state"]);
						g_currentUserFriendList.push_back(user);
					}
				}

				// 记录当前用户群组列表信息
				if (js.contains("groups")) {
					vector<string> vec1 = js["groups"];
					for (string &str : vec1) {
						json grpjs = json::parse(str);
						Group group;
						group.setId(grpjs["id"].get<int>());
						group.setName(grpjs["groupname"]);
						group.setDesc(grpjs["groupdesc"]);
						vector<string> vec2 = grpjs["users"];
						for (string str2 : vec2) {
							GroupUser user;
							json js = json::parse(str2);
							user.setId(js["id"].get<int>());
							user.setName(js["name"]);
							user.setState(js["state"]);
							user.setRole(js["role"]);
							group.getUsers().push_back(user);
						}
						g_currentUserGroupList.push_back(group);
					}
				}

				// 显示当前用户的基本信息
				showcurrentUserData();

				// 显示当前用户的离线消息，个人聊天信息或群组消息
				if (js.contains("offlinemsg")) {
					vector<string> vec = js["offlinemsg"];
					for (string &str : vec) {
						json js = json::parse(str);
						if (ONE_CHAT_MSG == js["msgid"].get<int>()) {
							cout << js["time"] << " [ " << js["id"] << " ] " << js["name"] << " said: " << js["message"]
								 << endl;
						} else {
							cout << "group msg [" << js["groupid"].get<int>() << "] " << js["time"] << " [ " << js["id"]
								 << " ] " << js["name"] << " said: " << js["message"] << endl;
						}
					}
				}
			}

			sem_post(&rwsem);
			continue;
		}

		if (REG_MSG_ACK == messagetype) {
			if (0 != js["errno"].get<int>()) {
				cerr << "register error!" << endl;
			} else {
				cout << "register success, userid is " << js["id"].get<int>() << endl;
			}

			sem_post(&rwsem);
			continue;
		}

		if (ONE_CHAT_MSG == messagetype) {
			cout << js["time"] << " [ " << js["id"] << " ] " << js["name"] << " said: " << js["message"] << endl;
			continue;
		}
		if (GROUP_CHAT_MSG == messagetype) {
			cout << "group msg [" << js["groupid"].get<int>() << "] " << js["time"] << " [ " << js["id"] << " ] "
				 << js["name"] << " said: " << js["message"] << endl;
			continue;
		}
		if (LOGOUT_MSG_ACK == messagetype) {
			if (js["errno"].get<int>() == 0) {
				cout << "logout success" << endl;

				g_currentUserFriendList.clear();
				g_currentUserGroupList.clear();
				g_isLoginSuccess = false;

				isMainMenuRunning = false;
			} else {
				cerr << js["errmsg"] << endl;
			}

			continue;
		}
	}
}

void showcurrentUserData() {
	cout << "====================login user====================" << endl;
	cout << " Current login user =>id: " << g_currentUser.getId() << " name: " << g_currentUser.getName() << endl;
	cout << "====================Friend List===================" << endl;
	if (!g_currentUserFriendList.empty()) {
		for (User &user : g_currentUserFriendList) {
			cout << user.getId() << " " << user.getName() << " " << user.getState() << endl;
		}
	}
	cout << "====================Group List===================" << endl;

	if (!g_currentUserGroupList.empty()) {
		for (Group &group : g_currentUserGroupList) {
			cout << group.getId() << " " << group.getName() << " " << group.getDesc() << " " << endl;
			for (GroupUser &groupuser : group.getUsers()) {
				cout << groupuser.getId() << " " << groupuser.getName() << " " << groupuser.getState() << " "
					 << groupuser.getRole() << endl;
			}
		}
	}
	cout << "==================================================";
}

void help(int fd = 0, string = "");
void chat(int, string);
void addfriend(int, string);
void creatgroup(int, string);
void addgroup(int, string);
void groupchat(int, string);
void logout(int, string);

// 系统支持的客户端命令列表
unordered_map<string, string> commandMap = {{"help", "显示所有支持的命令,格式: help"},
											{"chat", "一对一聊天,格式: chat:friendid:message"},
											{"addfriend", "添加好友,格式: addfriend:friendid"},
											{"creatgroup", "创建群组,格式: creatgroup:groupname:groupdesc"},
											{"addgroup", "加入群组,格式: addgroup:groupid"},
											{"groupchat", "群聊,格式: groupchat:groupid:message"},
											{"logout", "注销,格式:quit"}};

// 注册系统支持客户端命令处理
unordered_map<string, function<void(int, string)>> commandHandlerMap = {
	{"help", help},			{"chat", chat},			  {"addfriend", addfriend}, {"creatgroup", creatgroup},
	{"addgroup", addgroup}, {"groupchat", groupchat}, {"logout", logout}};

void mainMenu(int clientfd) {
	help();

	char buffer[1024] = {0};
	while (isMainMenuRunning) {
		cin.getline(buffer, 1024);
		string commandbuf(buffer);
		string command;
		int idx = commandbuf.find(":");
		if (-1 == idx) {
			command = commandbuf;
		} else {
			command = commandbuf.substr(0, idx);
		}
		auto it = commandHandlerMap.find(command);
		if (it == commandHandlerMap.end()) {
			cerr << " invalid input command " << endl;
			continue;
		}
		// 调用相应命令的时间回调，mainmenu对封闭修改，添加新功能不需要修改该函数
		it->second(clientfd, commandbuf.substr(idx + 1, commandbuf.size() - idx));
	}
}

// "help" command handler
void help(int, string) {
	cout << "<<< Show Command List >>>" << endl;
	for (auto &p : commandMap) {
		cout << p.first << " : " << p.second << endl;
	}
	cout << endl;
}

//"addfriend" command handler
void addfriend(int clientfd, string str) {
	int friendid = atoi(str.c_str());
	json js;
	js["msgid"] = ADD_FRIEND_MSG;
	js["id"] = g_currentUser.getId();
	js["friendid"] = friendid;
	string buffer = js.dump();

	// 客户端发送信息
	int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
	if (-1 == len) {
		cerr << " Send addfriend msg ERROR " << endl;
	}
}

//"chat" command Handler
void chat(int clientfd, string str) {
	int idx = str.find(":");
	if (-1 == idx) {
		cerr << " chat command ERROR " << endl;
		return;
	}
	int friendid = atoi(str.substr(0, idx).c_str());
	string message = str.substr(idx + 1, str.length() - idx);

	json js;
	js["msgid"] = ONE_CHAT_MSG;
	js["id"] = g_currentUser.getId();
	js["name"] = g_currentUser.getName();
	js["toid"] = friendid;
	js["message"] = message;
	string buffer = js.dump();

	int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
	if (-1 == len) {
		cerr << " send chat message ERROR" << endl;
	}
}

//"creatgroup" command handler
void creatgroup(int clientfd, string str) {
	int idx = str.find(":");
	if (-1 == idx) {
		cerr << " chat command ERROR " << endl;
		return;
	}
	string groupname = str.substr(0, idx);
	string groupdesc = str.substr(idx + 1);

	json js;
	js["msgid"] = CREATE_GROUP_MSG;
	js["id"] = g_currentUser.getId();
	js["groupname"] = groupname;
	js["groupdesc"] = groupdesc;
	string buffer = js.dump();

	int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
	if (-1 == len) {
		cerr << " creat group ERROR " << endl;
	}
}

//"addgroup" command handler
void addgroup(int clientfd, string str) {
	int groupid = atoi(str.c_str());

	json js;
	js["msgid"] = ADD_GROUP_MSG;
	js["id"] = g_currentUser.getId();
	js["groupid"] = groupid;
	string buffer = js.dump();

	int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
	if (-1 == len) {
		cerr << " add group ERROR " << endl;
	}
}

//"groupchat" command handler
void groupchat(int clientfd, string str) {
	int idx = str.find(":");
	if (-1 == idx) {
		cerr << " groupchat command ERROR " << endl;
		return;
	}
	int groupid = atoi(str.substr(0, idx).c_str());
	string message = str.substr(idx + 1);

	json js;
	js["msgid"] = GROUP_CHAT_MSG;
	js["id"] = g_currentUser.getId();
	js["name"] = g_currentUser.getName();
	js["groupid"] = groupid;
	js["message"] = message;
	string buffer = js.dump();

	int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
	if (-1 == len) {
		cerr << " send group message ERROR " << endl;
	}
}

//"logout" command handler
void logout(int clientfd, string str) {
	json js;
	js["msgid"] = LOGOUT_MSG;
	js["id"] = g_currentUser.getId();
	string buffer = js.dump();

	int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
	if (-1 == len) {
		cerr << " send logout message ERROR " << endl;
	} else {
		isMainMenuRunning = false;
	}
}
