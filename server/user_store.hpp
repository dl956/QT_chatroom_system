#pragma once
#include <string>
#include <unordered_map>
#include <mutex>

// very simple user store (in-memory). Passwords stored as plain text here (demo only).
class UserStore {
public:
    bool register_user(const std::string& username, const std::string& password);
    bool check_login(const std::string& username, const std::string& password);

private:
    std::mutex users_mutex_;
    std::unordered_map<std::string, std::string> user_password_map_;
};
