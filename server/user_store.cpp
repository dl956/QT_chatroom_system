// user_store.cpp
#include "user_store.hpp"
#include "logger.hpp"

bool UserStore::register_user(const std::string& username, const std::string& password) {
    std::lock_guard<std::mutex> lk(users_mutex_);
    if (username.empty()) {
        Logger::instance().warn("Register failed - empty username");
        return false;
    }
    if (user_password_map_.count(username)) {
        Logger::instance().warn("Register failed - exists", { {"username", username} });
        return false;
    }
    // Basic password check - do NOT log the password
    if (password.size() < 3) {
        Logger::instance().warn("Register failed - password too short", { {"username", username} });
        return false;
    }
    user_password_map_[username] = password;
    Logger::instance().info("User registered", { {"username", username}, {"total_users", static_cast<uint64_t>(user_password_map_.size())} });
    return true;
}

bool UserStore::check_login(const std::string& username, const std::string& password) {
    std::lock_guard<std::mutex> lk(users_mutex_);
    auto it = user_password_map_.find(username);
    if (it == user_password_map_.end()) {
        Logger::instance().warn("Login failed - no such user", { {"username", username} });
        return false;
    }
    bool ok = it->second == password;
    Logger::instance().info("Login attempt", { {"username", username}, {"ok", ok} });
    return ok;
}
