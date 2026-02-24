#pragma once
#include <string>

struct User {
    int id = 0;
    std::string username;
    std::string passwordHash;
    std::string salt;
    std::string nickname;
    std::string createdAt;
    std::string updatedAt;
};
