// message_store.cpp
#include "message_store.hpp"
#include "logger.hpp"

void MessageStore::push(const ChatMsg& m) {
    std::lock_guard<std::mutex> lk(mtx_);
    msgs_.push_back(m);
    Logger::instance().debug("Message pushed to store", { {"from", m.from}, {"to", m.to}, {"ts", m.ts} });
    if (msgs_.size() > 10000) {
        msgs_.erase(msgs_.begin(), msgs_.begin() + 1000);
        Logger::instance().info("Message store trimmed", { {"new_size", static_cast<uint64_t>(msgs_.size())} });
    }
}

std::vector<ChatMsg> MessageStore::recent(size_t n) {
    std::lock_guard<std::mutex> lk(mtx_);
    std::vector<ChatMsg> out;
    size_t start = (msgs_.size() > n) ? (msgs_.size() - n) : 0;
    for (size_t i = start; i < msgs_.size(); ++i) out.push_back(msgs_[i]);
    return out;
}

std::vector<ChatMsg> MessageStore::for_user(const std::string& user, size_t n) {
    std::lock_guard<std::mutex> lk(mtx_);
    std::vector<ChatMsg> out;
    for (auto it = msgs_.rbegin(); it != msgs_.rend() && out.size() < n; ++it) {
        if (it->to.empty() || it->to == user || it->from == user) out.push_back(*it);
    }
    std::reverse(out.begin(), out.end());
    return out;
}