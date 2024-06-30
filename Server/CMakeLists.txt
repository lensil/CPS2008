cmake_minimum_required(VERSION 3.0.0)
project(test VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)  
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) 

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
