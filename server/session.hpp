#pragma once
#include <memory>
#include <boost/asio.hpp>
#include <deque>
#include <string>
#include <vector>   // 
#include <cstdint>  // 
#include <nlohmann/json.hpp>

class Server; // forward

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(boost::asio::ip::tcp::socket socket, Server& server);
    void start();
    void deliver(const std::string& json_text);
    std::string username() const;

private:
    void do_read_header();
    void do_read_body(uint32_t body_len);
    void process_message(const nlohmann::json& j);
    void do_write();

    boost::asio::ip::tcp::socket socket_;
    Server& server_;
    std::vector<uint8_t> header_buf_;
    std::vector<uint8_t> message_body_buffer_;
    std::deque<std::vector<uint8_t>> outgoing_message_queue_;
    std::string session_username_;
};
