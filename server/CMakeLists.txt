cmake_minimum_required(VERSION 3.16)
project(chat_server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost REQUIRED COMPONENTS system thread)
find_package(nlohmann_json REQUIRED)

add_executable(chat_server
    main.cpp
    server.cpp
    session.cpp
    user_store.cpp
    message_store.cpp
    logger.cpp          
    protocol.hpp
    session.hpp
    server.hpp
    user_store.hpp
    message_store.hpp
    logger.hpp           
)

# Define Windows target macros for this target (do this after add_executable)
target_compile_definitions(chat_server PRIVATE _WIN32_WINNT=0x0601 WIN32_LEAN_AND_MEAN)

target_include_directories(chat_server PRIVATE ${Boost_INCLUDE_DIRS})
target_include_directories(chat_server PRIVATE ${nlohmann_json_INCLUDE_DIRS})
target_link_libraries(chat_server PRIVATE Boost::system Boost::thread nlohmann_json::nlohmann_json ws2_32)