#include "groupMuduo.hpp"

#include <iostream>

#include "db.h"

// 创建群聊方法
bool groupMuduo::creatGroup(Group &group) {
	Mysql mysql;
	if (!mysql.connect()) {
		cerr << "Failed to connect to database" << endl;
		return false;
	}
	char sql[1024] = {0};
	sprintf(sql, "INSERT INTO AllGroup(groupname,groupdesc) VALUES('%s','%s')", group.getName().c_str(),
			group.getDesc().c_str());
	if (mysql.update(sql)) {
		group.setId(mysql_insert_id(mysql.getConnection()));
		return true;
	}
	return false;
}

// 加入群聊方法
void groupMuduo::addGroup(int userid, int groupid, std::string role) {
	Mysql mysql;
	if (!mysql.connect()) {
		cerr << "Failed to connect to database" << endl;
		return;
	}
	char sql[1024] = {0};
	sprintf(sql, "INSERT INTO GroupUser(groupid,grouprole,userid) VALUES('%d','%s','%d')", groupid, role.c_str(),
			userid);
	mysql.update(sql);
}

// 查询用户所在群组信息
std::vector<Group> groupMuduo::queryGroups(int userid) {
	Mysql mysql;
	std::vector<Group> groupvec;
	std::vector<Group> groupList;
	if (!mysql.connect()) {
		cerr << "Failed to connect to database" << endl;
	}

	/*
		先根据用户id在GroupUser表中查询该用户所属的群组信息
		再根据群组信息，查询属于该群组的所有用户的userid，并且和user表进行联合查询，查出用户的详细信息
	*/
	char sql[1024] = {0};
	sprintf(sql, "select a.id,a.groupname,a.groupdesc FROM AllGroup a INNER join \
        GroupUser b on a.id=b.groupid where b.userid = %d",
			userid);
	MYSQL_RES *res = mysql.query(sql);
	if (res != nullptr) {
		MYSQL_ROW row;
		while ((row = mysql_fetch_row(res)) != nullptr) {
			Group group;
			group.setId(atoi(row[0]));
			group.setName(row[1]);
			group.setDesc(row[2]);
			groupvec.push_back(group);
		}
		mysql_free_result(res);
	}
	// 查询群组的用户信息
	for (Group &group : groupvec) {
		sprintf(sql, "select a.id,a.name,a.state,b.grouprole FROM user a INNER join \
        GroupUser b on a.id=b.userid where b.groupid = %d",
				group.getId());

		MYSQL_RES *res = mysql.query(sql);
		if (res != nullptr) {
			MYSQL_ROW row;
			while ((row = mysql_fetch_row(res)) != nullptr) {
				GroupUser user;
				user.setId(atoi(row[0]));
				user.setName(row[1]);
				user.setState(row[2]);
				user.setRole(row[3]);
				group.getUsers().push_back(user);
			}
			mysql_free_result(res);
		}
	}
	return groupvec;
}

// 根据指定的groupid查询群组用户id列表，除userid自己，主要用户群聊业务给群组其他成员发信息
std::vector<int> groupMuduo::queryGroupUsers(int userid, int groupid) {
	Mysql mysql;
	std::vector<int> idvec;

	if (!mysql.connect()) {
		cerr << "Failed to connect to database" << endl;
		return idvec;
	}

	char sql[1024] = {0};
	sprintf(sql, "SELECT userid FROM GroupUser WHERE groupid='%d' && userid != '%d'", groupid, userid);
	MYSQL_RES *res = mysql.query(sql);
	if (res != nullptr) {
		MYSQL_ROW row;
		while ((row = mysql_fetch_row(res)) != nullptr) {
			idvec.push_back(atoi(row[0]));
		}
		mysql_free_result(res);
	}
	return idvec;
}
