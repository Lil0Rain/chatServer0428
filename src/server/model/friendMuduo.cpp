#include "friendMuduo.hpp"

#include <iostream>

#include "db.h"

void friendMuduo::addFriend(int userid, int friendid) {
	Mysql mysql;
	if (!mysql.connect()) {
		std::cerr << "Failed to connect to database" << std::endl;
		return;
	}
	char sql[1024];
	sprintf(sql, "INSERT INTO friend(userid,friendid) VALUES('%d','%d')", userid, friendid);
	mysql.update(sql);
}

std::vector<User> friendMuduo::query(int userid) {
	std::vector<User> friends;
	// 1. 连接数据库
	Mysql mysql;
	if (!mysql.connect()) {
		cerr << "Failed to connect to database" << endl;
		return friends;
	}
	// 2. 构造SQL语句
	char sql[1024] = {0};
	sprintf(sql,
			"SELECT u.id, u.name, u.state FROM user u INNER JOIN friend f ON u.id = f.friendid WHERE f.userid = %d",
			userid);

	// 3. 执行SQL语句
	MYSQL_RES *res = mysql.query(sql);
	if (res != nullptr) {
		// 将查询结果的每一行数据封装成一个user对象，并添加到friends列表中返回
		MYSQL_ROW row;
		while ((row = mysql_fetch_row(res)) != nullptr) {
			User friendUser;
			friendUser.setId(atoi(row[0]));
			friendUser.setName(row[1]);
			friendUser.setState(row[2]);
			friends.push_back(friendUser);
		}
	}
	mysql_free_result(res);
	return friends;
}