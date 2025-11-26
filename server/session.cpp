// session.cpp
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>

#include "session.hpp"
#include "server.hpp"
#include "protocol.hpp"
#include "logger.hpp"
#include <chrono>
#include <nlohmann/json.hpp>
#include <algorithm>

using json = nlohmann::json;
namespace asio = boost::asio;

static std::string preview_text(const std::string& s, size_t maxlen = 200) {
    if (s.size() <= maxlen) return s;
    return s.substr(0, maxlen) + "...";
}

// Create a redacted copy of incoming JSON suitable for logs:
// - replace "password" with "<REDACTED>"
// - replace "text" with preview
// - keep username, type, etc.
static json redact_for_logging(const std::string& raw) {
    try {
        json j = json::parse(raw);
        if (j.contains("password")) j["password"] = "<REDACTED>";
        if (j.contains("text")) j["text"] = preview_text(j["text"].get<std::string>(), 200);
        return j;
    } catch (...) {
        json out;
        out["raw_preview"] = preview_text(raw, 200);
        return out;
    }
}

Session::Session(asio::ip::tcp::socket socket, Server& server)
    : socket_(std::move(socket)), server_(server), header_buf_(4) {
    Logger::instance().debug("Session constructed");
}

void Session::start() {
    Logger::instance().info("Session start");
    do_read_header();
}

void Session::do_read_header() {
    auto self = shared_from_this();
    asio::async_read(socket_, asio::buffer(header_buf_), [this, self](std::error_code ec, std::size_t) {
        if (ec) {
            server_.on_disconnect(self);
            Logger::instance().info("Session read header error/disconnect", { {"ec", ec.message()}, {"user", username_} });
            return;
        }
        uint32_t len = parse_length(header_buf_);
        if (len == 0) { do_read_header(); return; }
        do_read_body(len);
    });
}

void Session::do_read_body(uint32_t body_len) {
    body_buf_.assign(body_len, 0);
    auto self = shared_from_this();
    asio::async_read(socket_, asio::buffer(body_buf_), [this, self](std::error_code ec, std::size_t) {
        if (ec) {
            server_.on_disconnect(self);
            Logger::instance().info("Session read body error/disconnect", { {"ec", ec.message()}, {"user", username_} });
            return;
        }
        std::string s(body_buf_.begin(), body_buf_.end());

        // Log a redacted/preview copy of the JSON so we can see content without exposing passwords
        json redacted = redact_for_logging(s);
        Logger::instance().debug("Received JSON", { {"from", username_}, {"json_len", static_cast<uint64_t>(s.size())}, {"payload", redacted} });

        try {
            json j = json::parse(s);
            process_message(j);
        } catch (const std::exception& ex) {
            Logger::instance().error("Bad JSON parse", { {"what", ex.what()}, {"payload_preview", preview_text(s, 200)} });
        }
        do_read_header();
    });
}

void Session::process_message(const json& j) {
    // message types: register, login, message, private, history, heartbeat, list_users
    std::string type = j.value("type", "");
    Logger::instance().debug("Processing message", { {"type", type}, {"user", username_} });

    if (type == "register") {
        std::string user = j.value("username", "");
        std::string pass = j.value("password", "");
        bool ok = server_.user_store().register_user(user, pass);
        json r = { {"type","register_result"}, {"ok", ok} };
        if (!ok) {
            r["reason"] = "username_exists";
            Logger::instance().warn("Register failed", { {"username", user}, {"reason", "username_exists"} });
        } else {
            Logger::instance().info("User registered (via session)", { {"username", user} });
        }
        deliver(r.dump());

    } else if (type == "login") {
        std::string user = j.value("username", "");
        std::string pass = j.value("password", "");
        bool ok = server_.user_store().check_login(user, pass);
        json r = { {"type","login_result"}, {"ok", ok} };
        if (!ok) {
            r["reason"] = "invalid";
            Logger::instance().warn("Login failed", { {"username", user}, {"reason", "invalid"} });
        } else {
            username_ = user;
            server_.on_login(shared_from_this(), user);
            Logger::instance().info("Login success", { {"username", user} });
			r["username"] = user;
        }
		Logger::instance().info("login_result JSON", {{"json", r.dump()}});
        deliver(r.dump());
        if (ok) {
            // send recent history
            auto hs = server_.message_store().for_user(user, 100);
            for (auto& m : hs) {
                json mj = {
                    {"type", m.to.empty() ? "message" : "private"},
                    {"from", m.from},
                    {"to", m.to},
                    {"text", m.text},
                    {"ts", m.ts}
                };
                deliver(mj.dump());
            }
        }

    } else if (type == "message") {
        // Reject messages from not-logged-in sessions
        if (username_.empty()) {
            json err = { {"type", "error"}, {"error", "not_logged_in"} };
            deliver(err.dump());
            Logger::instance().warn("Message rejected - not logged in");
            return;
        }

        std::string text = j.value("text", "");
        uint64_t ts = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
        ChatMsg cm{ username_, "", text, ts };
        server_.message_store().push(cm);

        // broadcast to all INCLUDING sender (so sender will also receive the canonical message)
        json mj = { {"type","message"}, {"from", cm.from}, {"text", cm.text}, {"ts", cm.ts} };
        server_.broadcast(mj.dump()); // do not exclude sender

        // Log a preview at INFO and the full text at DEBUG
        Logger::instance().info("Broadcast message", { {"from", cm.from}, {"len", static_cast<uint64_t>(text.size())}, {"text_preview", preview_text(text, 200)} });
        Logger::instance().debug("Broadcast full message", { {"from", cm.from}, {"text", text} });

    } else if (type == "private") {
        // Reject private message if not logged in
        if (username_.empty()) {
            json err = { {"type", "error"}, {"error", "not_logged_in"} };
            deliver(err.dump());
            Logger::instance().warn("Private message rejected - not logged in");
            return;
        }

        std::string to = j.value("to", "");
        std::string text = j.value("text", "");
        uint64_t ts = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
        ChatMsg cm{ username_, to, text, ts };
        server_.message_store().push(cm);

        json mj = { {"type","private"}, {"from", cm.from}, {"to", cm.to}, {"text", cm.text}, {"ts", cm.ts} };
        server_.send_to_user(to, mj.dump());
        // also deliver to sender
        deliver(mj.dump());

        Logger::instance().info("Private message", { {"from", cm.from}, {"to", cm.to}, {"len", static_cast<uint64_t>(text.size())}, {"text_preview", preview_text(text, 200)} });
        Logger::instance().debug("Private message full", { {"from", cm.from}, {"to", cm.to}, {"text", text} });

    } else if (type == "heartbeat") {
        json r = { {"type","pong"} };
        deliver(r.dump());

    } else if (type == "history") {
        size_t n = j.value("n", 50);
        auto hs = server_.message_store().for_user(username_, n);
        for (auto& m : hs) {
            json mj = {
                {"type", m.to.empty() ? "message" : "private"},
                {"from", m.from},
                {"to", m.to},
                {"text", m.text},
                {"ts", m.ts}
            };
            deliver(mj.dump());
        }

    } else if (type == "list_users") {
        // respond with current online users to this session only
        auto users = server_.online_usernames();
        json r;
        r["type"] = "user_list";
        r["users"] = users;
        deliver(r.dump());

    } else if (type == "logout") {
        Logger::instance().info("User requested logout", { {"username", username_} });
        socket_.close();
        return;
    } else {
        Logger::instance().warn("Unknown message type", { {"type", type} });
    }
}

void Session::deliver(const std::string& json_text) {
    auto frame = make_frame(json_text);
    bool writing = !write_queue_.empty();
    write_queue_.push_back(std::move(frame));
    if (!writing) do_write();
}

void Session::do_write() {
    auto self = shared_from_this();
    boost::asio::async_write(socket_, boost::asio::buffer(write_queue_.front()), [this, self](std::error_code ec, std::size_t) {
        if (ec) {
            server_.on_disconnect(self);
            Logger::instance().info("Session write error/disconnect", { {"ec", ec.message()}, {"user", username_} });
            return;
        }
        write_queue_.pop_front();
        if (!write_queue_.empty()) do_write();
    });
}

std::string Session::username() const { return username_; }