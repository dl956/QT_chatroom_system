cmake_minimum_required(VERSION 3.16)
project(qt_chat_client LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
find_package(Qt6 REQUIRED COMPONENTS Quick Network)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

add_executable(qt_chat_client
    main.cpp
    tcpclient.h
    tcpclient.cpp
    messagemodel.h
    messagemodel.cpp
    qml.qrc
    main.qml
)

target_link_libraries(qt_chat_client PRIVATE Qt6::Quick Qt6::Network)
