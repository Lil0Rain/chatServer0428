#pragma  once
#ifndef GROUPUSER_HPP
#define GROUPUSER_HPP
#include "user.hpp"

#include <string>

class GroupUser : public User
{
    public:
    void setRole(std::string role){this->role = role;}
    const std::string getRole() const{return this->role;}
    private:
    std::string role;
};

#endif