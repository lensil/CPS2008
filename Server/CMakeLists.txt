cmake_minimum_required(VERSION 3.0.0)
project(test VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# It's better to check for compiler features rather than hardcoding the compiler
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9)
        message(FATAL_ERROR "GCC version must be at least 9!")
    endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9)
        message(FATAL_ERROR "Clang version must be at least 9!")
    endif()
else()
    message(WARNING "You are using an unsupported compiler! Compilation has only been tested with Clang and GCC.")
endif()

add_executable(server 
    main.cpp 
    Server.cpp 
    Server.h
    Client.cpp 
    Client.h 
    DisconnectedClient.h 
    Commands.cpp
    Commands.h
    Canvas.cpp
    Canvas.h
)

# Add threading support
find_package(Threads REQUIRED)
target_link_libraries(server PRIVATE Threads::Threads)