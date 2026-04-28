#include "json.hpp"
using json = nlohmann::json;

#include <iostream>
#include <string>
#include <vector>
#include <map>

using namespace std;

//json序列化示例1
string func1(){
    json js;
    js["msg_type"] = 2;
    js["from"] = "zhangsan";
    js["to"] = "lisi";
    js["msg"] = "nihao!";

    string sendbuf = js.dump();//dump，将json数据对象--->序列化为json字符串
    // cout<<sendbuf.c_str()<<endl;
    return sendbuf;
}

//json序列化示例2
string func2(){
    json js ;
    js["id"] = {1,2,3,4,5};
    js["name"] = "zhang san";
    js["msg"]["zhang san"] = "hello world";
    js["msg"]["li si"] = "helllo China";
    //上面
    js["msg"] = {{"zhang san","hello world"},{"li si","hello China"}};
    // cout<<js<<endl;
    return js.dump();
}

//json序列化示例2
string func3(){
    json js;
    vector<int> vec;
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(5);

    js["list"] = vec;

    map<int,string> m;
    m.insert({1,"黄山"});
    m.insert({2,"华山"});
    m.insert({3,"泰山"});
    m.insert({4,"嵩山"});

    js["path"] = m;

    // cout<<js<<endl;
    return js.dump();

}

// int main(){
//     // func1();
//     // func2()
//     // func3();
//     string  recvbuf = func3();
//     //数据的反序列化  json字符串反序列化为数据对象（看作容器方便访问）
//     json jsbuf = json::parse(recvbuf);

//     // cout<<jsbuf["msg_type"]<<endl;
//     // cout<<jsbuf["from"]<<endl;
//     // cout<<jsbuf["to"]<<endl;
//     // cout<<jsbuf["msg"]<<endl;

//     // cout<<jsbuf["id"]<<endl;
//     // cout<<jsbuf["name"]<<endl;
//     // cout<<jsbuf["msg"]<<endl;

//     cout<<jsbuf["list"]<<endl;
//     cout<<jsbuf["path"]<<endl;

//     return 0;
// }