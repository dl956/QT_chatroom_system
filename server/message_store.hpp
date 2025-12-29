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
    void add_message(const ChatMsg& chat_message);
    std::vector<ChatMsg> get_recent_messages(size_t count = 50);
    std::vector<ChatMsg> get_messages_for_user(const std::string& user, size_t count = 50);
private:
    std::mutex messages_mutex_;
    std::vector<ChatMsg> message_buffer_;
};
