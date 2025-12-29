// logger.cpp
#include "logger.hpp"
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <cstdlib>
#include <cctype>

namespace fs = std::filesystem;

Logger& Logger::instance() {
    static Logger inst;
    return inst;
}

Logger::Logger()
    : log_file_path_("logs/server.log"),
      log_level_(LogLevel::Info),
      max_log_file_size_(10ull * 1024 * 1024),
      log_rotate_count_(5),
      is_initialized_(false) {
    // leave stream closed until init or first write
}

Logger::~Logger() {
    std::lock_guard<std::mutex> lock(file_mutex_);
    if (log_file_stream_.is_open()) log_file_stream_.close();
}

void Logger::init(const std::string& log_file_path, LogLevel log_level, std::uint64_t max_size_bytes, int rotate_count) {
    std::lock_guard<std::mutex> lock(file_mutex_);
    log_file_path_ = log_file_path;
    log_level_ = log_level;
    max_log_file_size_ = max_size_bytes;
    log_rotate_count_ = rotate_count;

    fs::path dir = fs::path(log_file_path_).parent_path();
    if (!dir.empty() && !fs::exists(dir)) {
        std::error_code ec;
        fs::create_directories(dir, ec);
        (void)ec;
    }

    if (log_file_stream_.is_open()) log_file_stream_.close();
    log_file_stream_.open(log_file_path_, std::ios::app);
    is_initialized_ = true;
}

std::string Logger::level_to_string(LogLevel log_level) const {
    switch (log_level) {
        case LogLevel::Debug: return "debug";
        case LogLevel::Info:  return "info";
        case LogLevel::Warn:  return "warn";
        case LogLevel::Err: return "error";
        default: return "info";
    }
}

std::string Logger::timestamp_iso() const {
    using namespace std::chrono;
    auto now = system_clock::now();
    auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;

    std::time_t current_time = system_clock::to_time_t(now);
    std::tm tm;
#ifdef _WIN32
    gmtime_s(&tm, &current_time);
#else
    gmtime_r(&current_time, &tm);
#endif
    std::ostringstream time_stream;
    time_stream << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S");
    time_stream << '.' << std::setw(3) << std::setfill('0') << ms.count() << "Z";
    return time_stream.str();
}

void Logger::rotate_if_needed_locked() {
    if (!log_file_stream_.is_open()) {
        log_file_stream_.open(log_file_path_, std::ios::app);
        if (!log_file_stream_.is_open()) return;
    }

    std::error_code ec;
    auto sz = fs::file_size(log_file_path_, ec);
    if (ec) return;
    if (sz < max_log_file_size_) return;

    // close current
    log_file_stream_.close();

    // rotate: move file -> file.1, file.1 -> file.2, ... keep log_rotate_count_
    for (int i = log_rotate_count_ - 1; i >= 0; --i) {
        fs::path src = (i == 0) ? fs::path(log_file_path_) : fs::path(log_file_path_ + "." + std::to_string(i));
        fs::path dst = fs::path(log_file_path_ + "." + std::to_string(i + 1));
        if (fs::exists(src, ec)) {
            if (fs::exists(dst, ec)) fs::remove(dst, ec);
            fs::rename(src, dst, ec);
            (void)ec;
        }
    }

    // reopen new file
    log_file_stream_.open(log_file_path_, std::ios::app);
}

void Logger::log(LogLevel log_level, const std::string& log_message, const nlohmann::json& extra) {
    // quick log_level check (no full lock)
    if (static_cast<int>(log_level) < static_cast<int>(log_level_)) return;

    std::lock_guard<std::mutex> lock(file_mutex_);
    if (!is_initialized_) {
        // auto init from env if not explicitly inited
        const char* env_file = std::getenv("LOG_FILE");
        std::string file = env_file ? env_file : log_file_path_;
        const char* env_level = std::getenv("LOG_LEVEL");
        LogLevel lvl_env = log_level_;
        if (env_level) {
            std::string level_string(env_level);
            for (auto &c: level_string) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            if (level_string == "debug") lvl_env = LogLevel::Debug;
            else if (level_string == "info") lvl_env = LogLevel::Info;
            else if (level_string == "warn") lvl_env = LogLevel::Warn;
            else if (level_string == "error") lvl_env = LogLevel::Err;
        }
        const char* env_max = std::getenv("LOG_MAX_SIZE");
        std::uint64_t maxsz = max_log_file_size_;
        if (env_max) {
            try { maxsz = static_cast<std::uint64_t>(std::stoull(env_max)); } catch(...) {}
        }
        const char* env_rot = std::getenv("LOG_ROTATE_COUNT");
        int rc = log_rotate_count_;
        if (env_rot) {
            try { rc = std::stoi(env_rot); } catch(...) {}
        }
        init(file, lvl_env, maxsz, rc);
    }

    rotate_if_needed_locked();

    nlohmann::json log_entry;
    log_entry["timestamp"] = timestamp_iso();
    log_entry["log_level"] = level_to_string(log_level);
    const char* svc = std::getenv("SERVICE_NAME");
    log_entry["service"] = svc ? svc : "chat_server";
    log_entry["thread_id"] = std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id()));
    log_entry["log_message"] = log_message;
    if (!extra.is_null()) log_entry["extra"] = extra;

    if (log_file_stream_.is_open()) {
        log_file_stream_ << log_entry.dump() << "\n";
        log_file_stream_.flush();
    } else {
        std::fprintf(stderr, "%level_string\n", log_entry.dump().c_str());
    }
}

void Logger::debug(const std::string& log_message, const nlohmann::json& extra) { log(LogLevel::Debug, log_message, extra); }
void Logger::info(const std::string& log_message, const nlohmann::json& extra)  { log(LogLevel::Info,  log_message, extra); }
void Logger::warn(const std::string& log_message, const nlohmann::json& extra)  { log(LogLevel::Warn,  log_message, extra); }
void Logger::error(const std::string& log_message, const nlohmann::json& extra) { log(LogLevel::Err, log_message, extra); }
