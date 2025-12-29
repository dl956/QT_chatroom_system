// message_store.cpp
#include "message_store.hpp"
#include "logger.hpp"

void MessageStore::add_message(const ChatMsg& chat_message) {
    std::lock_guard<std::mutex> lk(messages_mutex_);
    message_buffer_.push_back(chat_message);
    Logger::instance().debug("Message pushed to store", { {"from", chat_message.from}, {"to", chat_message.to}, {"ts", chat_message.ts} });
    if (message_buffer_.size() > 10000) {
        message_buffer_.erase(message_buffer_.begin(), message_buffer_.begin() + 1000);
        Logger::instance().info("Message store trimmed", { {"new_size", static_cast<uint64_t>(message_buffer_.size())} });
    }
}

std::vector<ChatMsg> MessageStore::get_recent_messages(size_t count) {
    std::lock_guard<std::mutex> lk(messages_mutex_);
    std::vector<ChatMsg> out;
    size_t start = (message_buffer_.size() > count) ? (message_buffer_.size() - count) : 0;
    for (size_t i = start; i < message_buffer_.size(); ++i) out.push_back(message_buffer_[i]);
    return out;
}

std::vector<ChatMsg> MessageStore::get_messages_for_user(const std::string& user, size_t count) {
    std::lock_guard<std::mutex> lk(messages_mutex_);
    std::vector<ChatMsg> out;
    for (auto it = message_buffer_.rbegin(); it != message_buffer_.rend() && out.size() < count; ++it) {
        if (it->to.empty() || it->to == user || it->from == user) out.push_back(*it);
    }
    std::reverse(out.begin(), out.end());
    return out;
}
