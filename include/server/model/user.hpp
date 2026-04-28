#pragma once
#ifndef USER_H
#define USER_H

#include <string>
using namespace std;


//匹配user表的ORM类
class User
{
public:
    User(int id = -1, string name = "", string password = "", string state = "offline") : id(id), name(name), password(password), state(state) {}

    int getId() const { return id; }
    //对于复杂对象例如vector<string>，我们通常返回const引用以避免不必要的复制，但对于简单类型如int，直接返回值更合适。
    //get方法一般为const成员函数，承诺它们不会修改对象的状态。
    const string& getName() const { return name; }         
    const string& getPassword() const { return password; }
    const string& getState() const { return state; }

    void setId(int id) { this->id = id; }
    void setName(const string& name) { this->name = name; }
    void setPassword(const string& password) { this->password = password; }
    void setState(const string& state) { this->state = state; }

private:
    int id;
    string name;
    string password;
    string state;
};

#endif