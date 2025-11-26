#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <algorithm>

struct ChatMsg {
    std::string from;
    std::string to; // empty for group
    std::string text;
    uint64_t ts; // epoch ms
};

class MessageStore {
public:
    void push(const ChatMsg& m);
    std::vector<ChatMsg> recent(size_t n = 50);
    std::vector<ChatMsg> for_user(const std::string& user, size_t n = 50);
private:
    std::mutex mtx_;
    std::vector<ChatMsg> msgs_;
};
