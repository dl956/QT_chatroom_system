// logger.hpp
#pragma once
#include <string>
#include <mutex>
#include <fstream>
#include <chrono>
#include <thread>
#include <nlohmann/json.hpp>

enum class LogLevel { Debug = 0, Info = 1, Warn = 2, Err = 3 };

class Logger {
public:
    static Logger& instance();

    // init must be called before first use (but instance() will create a usable default too)
    void init(const std::string& log_file_path,
              LogLevel log_level = LogLevel::Info,
              std::uint64_t max_size_bytes = 10ull * 1024 * 1024, // 10MB
              int rotate_count = 5);

    void log(LogLevel log_level, const std::string& log_message, const nlohmann::json& extra = nlohmann::json());

    void debug(const std::string& log_message, const nlohmann::json& extra = nlohmann::json());
    void info(const std::string& log_message, const nlohmann::json& extra = nlohmann::json());
    void warn(const std::string& log_message, const nlohmann::json& extra = nlohmann::json());
    void error(const std::string& log_message, const nlohmann::json& extra = nlohmann::json());

private:
    Logger();
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::string level_to_string(LogLevel log_level) const;
    std::string timestamp_iso() const;
    void rotate_if_needed_locked();

    std::mutex file_mutex_;
    std::ofstream log_file_stream_;
    std::string log_file_path_;
    LogLevel log_level_;
    std::uint64_t max_log_file_size_;
    int log_rotate_count_;
    bool is_initialized_;
};
