cmake_minimum_required(VERSION 3.16)
project(QtChatClient)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)

find_package(Qt5 REQUIRED COMPONENTS Widgets Network)

add_executable(QtChatClient
    main.cpp
    login_dialog.cpp
    chat_window.cpp
    ${CMAKE_SOURCE_DIR}/include/qt-client/login_dialog.h
    ${CMAKE_SOURCE_DIR}/include/qt-client/chat_window.h
)

target_include_directories(QtChatClient PRIVATE
    ${CMAKE_SOURCE_DIR}/include/qt-client
)

target_link_libraries(QtChatClient Qt5::Widgets Qt5::Network)
