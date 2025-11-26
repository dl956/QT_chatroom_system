// server.hpp
#pragma once
#include <boost/asio.hpp>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <string>
#include <vector>
#include "user_store.hpp"
#include "message_store.hpp"

class Session;

class Server {
public:
    Server(boost::asio::io_context& ioc, unsigned short port);
    void run_accept();
    void on_login(std::shared_ptr<Session> sess, const std::string& username);
    void on_disconnect(std::shared_ptr<Session> sess);
    void broadcast(const std::string& json_text, std::shared_ptr<Session> except = nullptr);
    void send_to_user(const std::string& username, const std::string& json_text);

    // new helpers for online users
    void broadcast_user_list();
    std::vector<std::string> online_usernames();

    UserStore& user_store() { return users_; }
    MessageStore& message_store() { return msg_store_; }

private:
    boost::asio::ip::tcp::acceptor acceptor_;
    boost::asio::io_context& ioc_;
    std::mutex mtx_;
    std::unordered_map<std::string, std::shared_ptr<Session>> online_;
    UserStore users_;
    MessageStore msg_store_;
};