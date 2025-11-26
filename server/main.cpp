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
        std::string logfile = env_logfile ? env_logfile : "logs/server.log";
        const char* env_level = std::getenv("LOG_LEVEL");
        LogLevel lvl = LogLevel::Info;
        if (env_level) {
            std::string s(env_level);
            for (auto &c: s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            if (s == "debug") lvl = LogLevel::Debug;
            else if (s == "warn") lvl = LogLevel::Warn;
            else if (s == "error") lvl = LogLevel::Err;
        }
        const char* env_max = std::getenv("LOG_MAX_SIZE");
        std::uint64_t maxsz = 10ull * 1024 * 1024;
        if (env_max) { try { maxsz = static_cast<std::uint64_t>(std::stoull(env_max)); } catch(...) {} }
        const char* env_rot = std::getenv("LOG_ROTATE_COUNT");
        int rot = 5;
        if (env_rot) { try { rot = std::stoi(env_rot); } catch(...) {} }

        Logger::instance().init(logfile, lvl, maxsz, rot);
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

            // run io_context on multiple threads (reactor threads)
            size_t threads = std::thread::hardware_concurrency();
            if (threads == 0) threads = 2;
            Logger::instance().info("Threads to run: ", { {"count", threads} });

            std::vector<std::thread> v;
            for (size_t i = 0; i < threads; ++i) {
                v.emplace_back([&ioc, i](){
                    try {
                        Logger::instance().info("Thread started", {{"id", i}});
                        ioc.run();
                        Logger::instance().info("Thread exit normally", {{"id", i}});
                    } catch (const std::exception& ex) {
                        Logger::instance().error("Thread exception", {{"id", i}, {"what", ex.what()}});
                    }
                });
            }
            Logger::instance().info("Server listening", { {"port", port}, {"threads", static_cast<uint64_t>(threads)} });

            for (auto& t : v) t.join();
            Logger::instance().info("All threads joined. Main ready to exit.");
        }

        Logger::instance().info("Main function end, process about to exit.");

    } catch (const std::exception& ex) {
        Logger::instance().error("Main thread exception caught", {{"what", ex.what()}});
    } catch (...) {
        Logger::instance().error("Main thread unknown exception caught");
    }

    return 0;
}