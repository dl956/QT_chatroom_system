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
    void init(const std::string& file_path,
              LogLevel level = LogLevel::Info,
              std::uint64_t max_size_bytes = 10ull * 1024 * 1024, // 10MB
              int rotate_count = 5);

    void log(LogLevel lvl, const std::string& message, const nlohmann::json& extra = nlohmann::json());

    void debug(const std::string& message, const nlohmann::json& extra = nlohmann::json());
    void info(const std::string& message, const nlohmann::json& extra = nlohmann::json());
    void warn(const std::string& message, const nlohmann::json& extra = nlohmann::json());
    void error(const std::string& message, const nlohmann::json& extra = nlohmann::json());

private:
    Logger();
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::string level_to_string(LogLevel l) const;
    std::string timestamp_iso() const;
    void rotate_if_needed_locked();

    std::mutex mtx_;
    std::ofstream ofs_;
    std::string file_path_;
    LogLevel level_;
    std::uint64_t max_size_;
    int rotate_count_;
    bool initialized_;
};