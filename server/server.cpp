// server.cpp
// Windows socket headers should come before other network includes to avoid macro/definition conflicts
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>

#include <boost/asio.hpp>
#include "server.hpp"
#include "session.hpp"
#include "logger.hpp"
#include <nlohmann/json.hpp>

namespace asio = boost::asio;
using tcp = asio::ip::tcp;
using json = nlohmann::json;

Server::Server(asio::io_context& ioc, unsigned short port)
    : acceptor_(ioc, tcp::endpoint(tcp::v4(), port)), ioc_(ioc) {
    Logger::instance().info("Server constructed", { {"port", port} });
}

void Server::run_accept() {
    acceptor_.async_accept([this](std::error_code ec, tcp::socket socket) {
        if (!ec) {
            auto s = std::make_shared<Session>(std::move(socket), *this);
            Logger::instance().info("New connection accepted");
            s->start();
        } else {
            Logger::instance().error("Accept error", { {"what", ec.message()}, {"value", ec.value()} });
        }
        run_accept();
    });
}

void Server::on_login(std::shared_ptr<Session> sess, const std::string& username) {
	{
		std::lock_guard<std::mutex> lk(online_users_mutex_);
		online_user_sessions_[username] = sess;
		Logger::instance().info("User logged in", { {"username", username}, {"online_count", static_cast<uint64_t>(online_user_sessions_.size())} });
	}
	// broadcast updated user list to everyone
    broadcast_user_list();
}

void Server::on_disconnect(std::shared_ptr<Session> sess) {
	{
		std::lock_guard<std::mutex> lk(online_users_mutex_);
		for (auto it = online_user_sessions_.begin(); it != online_user_sessions_.end();) {
			if (it->second == sess) {
				Logger::instance().info("User disconnected", { {"username", it->first} });
				it = online_user_sessions_.erase(it);
			} else ++it;
		}
	}
    // broadcast updated user list to everyone
    broadcast_user_list();
}

void Server::broadcast(const std::string& json_text, std::shared_ptr<Session> except) {
    std::lock_guard<std::mutex> lk(online_users_mutex_);
    Logger::instance().debug("Broadcasting message", { {"len", static_cast<uint64_t>(json_text.size())}, {"except", except ? except->username() : ""} });
    for (auto& kv : online_user_sessions_) {
        if (kv.second != except) kv.second->deliver(json_text);
    }
}

void Server::send_to_user(const std::string& username, const std::string& json_text) {
    std::lock_guard<std::mutex> lk(online_users_mutex_);
    auto it = online_user_sessions_.find(username);
    if (it != online_user_sessions_.end()) {
        it->second->deliver(json_text);
        Logger::instance().debug("Sent message to user", { {"to", username}, {"len", static_cast<uint64_t>(json_text.size())} });
    } else {
        Logger::instance().warn("User not online for send", { {"to", username} });
    }
}

void Server::broadcast_user_list() {
    json j;
    {
        std::lock_guard<std::mutex> lk(online_users_mutex_);
        j["type"] = "user_list";
        j["users"] = json::array();
        for (auto& kv : online_user_sessions_) {
            j["users"].push_back(kv.first);
        }
    }
    broadcast(j.dump());  // The lock online_users_mutex_ is no longer held here
}

std::vector<std::string> Server::online_usernames() {
    std::lock_guard<std::mutex> lk(online_users_mutex_);
    std::vector<std::string> out;
    out.reserve(online_user_sessions_.size());
    for (auto& kv : online_user_sessions_) out.push_back(kv.first);
    return out;
}
