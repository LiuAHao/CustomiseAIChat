#pragma once
#include <string>

struct Persona {
    int id = 0;
    int userId = 0;
    std::string name;
    std::string description;
    std::string createdAt;
};
