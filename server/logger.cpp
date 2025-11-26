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
    : file_path_("logs/server.log"),
      level_(LogLevel::Info),
      max_size_(10ull * 1024 * 1024),
      rotate_count_(5),
      initialized_(false) {
    // leave stream closed until init or first write
}

Logger::~Logger() {
    std::lock_guard<std::mutex> lk(mtx_);
    if (ofs_.is_open()) ofs_.close();
}

void Logger::init(const std::string& file_path, LogLevel level, std::uint64_t max_size_bytes, int rotate_count) {
    std::lock_guard<std::mutex> lk(mtx_);
    file_path_ = file_path;
    level_ = level;
    max_size_ = max_size_bytes;
    rotate_count_ = rotate_count;

    fs::path dir = fs::path(file_path_).parent_path();
    if (!dir.empty() && !fs::exists(dir)) {
        std::error_code ec;
        fs::create_directories(dir, ec);
        (void)ec;
    }

    if (ofs_.is_open()) ofs_.close();
    ofs_.open(file_path_, std::ios::app);
    initialized_ = true;
}

std::string Logger::level_to_string(LogLevel l) const {
    switch (l) {
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

    std::time_t t = system_clock::to_time_t(now);
    std::tm tm;
#ifdef _WIN32
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S");
    oss << '.' << std::setw(3) << std::setfill('0') << ms.count() << "Z";
    return oss.str();
}

void Logger::rotate_if_needed_locked() {
    if (!ofs_.is_open()) {
        ofs_.open(file_path_, std::ios::app);
        if (!ofs_.is_open()) return;
    }

    std::error_code ec;
    auto sz = fs::file_size(file_path_, ec);
    if (ec) return;
    if (sz < max_size_) return;

    // close current
    ofs_.close();

    // rotate: move file -> file.1, file.1 -> file.2, ... keep rotate_count_
    for (int i = rotate_count_ - 1; i >= 0; --i) {
        fs::path src = (i == 0) ? fs::path(file_path_) : fs::path(file_path_ + "." + std::to_string(i));
        fs::path dst = fs::path(file_path_ + "." + std::to_string(i + 1));
        if (fs::exists(src, ec)) {
            if (fs::exists(dst, ec)) fs::remove(dst, ec);
            fs::rename(src, dst, ec);
            (void)ec;
        }
    }

    // reopen new file
    ofs_.open(file_path_, std::ios::app);
}

void Logger::log(LogLevel lvl, const std::string& message, const nlohmann::json& extra) {
    // quick level check (no full lock)
    if (static_cast<int>(lvl) < static_cast<int>(level_)) return;

    std::lock_guard<std::mutex> lk(mtx_);
    if (!initialized_) {
        // auto init from env if not explicitly inited
        const char* env_file = std::getenv("LOG_FILE");
        std::string file = env_file ? env_file : file_path_;
        const char* env_level = std::getenv("LOG_LEVEL");
        LogLevel lvl_env = level_;
        if (env_level) {
            std::string s(env_level);
            for (auto &c: s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            if (s == "debug") lvl_env = LogLevel::Debug;
            else if (s == "info") lvl_env = LogLevel::Info;
            else if (s == "warn") lvl_env = LogLevel::Warn;
            else if (s == "error") lvl_env = LogLevel::Err;
        }
        const char* env_max = std::getenv("LOG_MAX_SIZE");
        std::uint64_t maxsz = max_size_;
        if (env_max) {
            try { maxsz = static_cast<std::uint64_t>(std::stoull(env_max)); } catch(...) {}
        }
        const char* env_rot = std::getenv("LOG_ROTATE_COUNT");
        int rc = rotate_count_;
        if (env_rot) {
            try { rc = std::stoi(env_rot); } catch(...) {}
        }
        init(file, lvl_env, maxsz, rc);
    }

    rotate_if_needed_locked();

    nlohmann::json j;
    j["timestamp"] = timestamp_iso();
    j["level"] = level_to_string(lvl);
    const char* svc = std::getenv("SERVICE_NAME");
    j["service"] = svc ? svc : "chat_server";
    j["thread_id"] = std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id()));
    j["message"] = message;
    if (!extra.is_null()) j["extra"] = extra;

    if (ofs_.is_open()) {
        ofs_ << j.dump() << "\n";
        ofs_.flush();
    } else {
        std::fprintf(stderr, "%s\n", j.dump().c_str());
    }
}

void Logger::debug(const std::string& message, const nlohmann::json& extra) { log(LogLevel::Debug, message, extra); }
void Logger::info(const std::string& message, const nlohmann::json& extra)  { log(LogLevel::Info,  message, extra); }
void Logger::warn(const std::string& message, const nlohmann::json& extra)  { log(LogLevel::Warn,  message, extra); }
void Logger::error(const std::string& message, const nlohmann::json& extra) { log(LogLevel::Err, message, extra); }