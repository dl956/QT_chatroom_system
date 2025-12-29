#include <iostream>
#include <boost/asio.hpp>
#include "server.hpp"
#include "logger.hpp"
#include <thread>

int main(int argc, char** argv) {
    try {
        unsigned short port = 9000;
        if (argc > 1) port = static_cast<unsigned short>(std::stoi(argv[1]));

        // initialize logger from env or default
        const char* env_logfile = std::getenv("LOG_FILE");
        std::string log_file_path = env_logfile ? env_logfile : "logs/server.log";
        const char* env_level = std::getenv("LOG_LEVEL");
        LogLevel log_level = LogLevel::Info;
        if (env_level) {
            std::string level_string(env_level);
            for (auto &c: level_string) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            if (level_string == "debug") log_level = LogLevel::Debug;
            else if (level_string == "warn") log_level = LogLevel::Warn;
            else if (level_string == "error") log_level = LogLevel::Err;
        }
        const char* env_max = std::getenv("LOG_MAX_SIZE");
        std::uint64_t maxsz = 10ull * 1024 * 1024;
        if (env_max) { try { maxsz = static_cast<std::uint64_t>(std::stoull(env_max)); } catch(...) {} }
        const char* env_rot = std::getenv("LOG_ROTATE_COUNT");
        int rotate_count = 5;
        if (env_rot) { try { rotate_count = std::stoi(env_rot); } catch(...) {} }

        Logger::instance().init(log_file_path, log_level, maxsz, rotate_count);
        Logger::instance().info("Logger initialized");

        boost::asio::io_context ioc;
        Logger::instance().info("io_context created");

        // Use a work_guard to prevent server from auto-terminating when no clients are connected
        boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_guard(ioc.get_executor());
        Logger::instance().info("work_guard created");

        {
            Logger::instance().info("Creating server object");
            Server server(ioc, port);
            Logger::instance().info("Server object constructed");

            server.run_accept();
            Logger::instance().info("Server run_accept called");

            // run io_context on multiple thread_count (reactor thread_count)
            size_t thread_count = std::thread::hardware_concurrency();
            if (thread_count == 0) thread_count = 2;
            Logger::instance().info("Threads to run: ", { {"count", thread_count} });

            std::vector<std::thread> io_threads;
            for (size_t thread_index = 0; thread_index < thread_count; ++thread_index) {
                io_threads.emplace_back([&ioc, thread_index](){
                    try {
                        Logger::instance().info("Thread started", {{"id", thread_index}});
                        ioc.run();
                        Logger::instance().info("Thread exit normally", {{"id", thread_index}});
                    } catch (const std::exception& ex) {
                        Logger::instance().error("Thread exception", {{"id", thread_index}, {"what", ex.what()}});
                    }
                });
            }
            Logger::instance().info("Server listening", { {"port", port}, {"thread_count", static_cast<uint64_t>(thread_count)} });

            for (auto& thread_obj : io_threads) thread_obj.join();
            Logger::instance().info("All thread_count joined. Main ready to exit.");
        }

        Logger::instance().info("Main function end, process about to exit.");

    } catch (const std::exception& ex) {
        Logger::instance().error("Main thread exception caught", {{"what", ex.what()}});
    } catch (...) {
        Logger::instance().error("Main thread unknown exception caught");
    }

    return 0;
}
