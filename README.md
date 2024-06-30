# NetSketch: A Collaborative Whiteboard
### CPS2008 Project

## Table of Contents
1. [Introduction](#introduction)
2. [System Requirements](#system-requirements)
3. [Usage](#usage)
4. [Project Structure](#project-structure)

## Introduction

NetSketch is a collaborative whiteboard application that allows multiple users to interact with a shared canvas over a network. This project was implemented for the CPS2008 unit at the University of Malta.

## System Requirements

- Server:
  - C++17 compatible compiler
  - CMake 3.0.0 or higher
  - POSIX-compliant operating system (Linux, macOS)
- Client:
  - Python 3.x
  - Tkinter library

## Usage

### Starting the Server

Run the server executable from the build directory:

```
./build/server
```

The server will start listening on port 6001 by default.

### Running the Client

1. Navigate to the Client directory
2. Run the client script:
   ```
   python3 client.py
   ```

3. The client will automatically connect to the server running on localhost:6001

## Project Structure

- Server:
    - `main.cpp`: Server entry point
    - `CMakeLists.txt`: CMake build configuration
    - `Server.cpp` / `Server.h`: Main server implementation
    - `Client.cpp` / `Client.h`: Client handling
    - `Commands.cpp` / `Commands.h`: Command processing
    - `Canvas.cpp` / `Canvas.h`: Canvas state management
    - `DisconnectedClient.h`: Disconnected clients
    
- Client:
    - `client.py`: Python client implementation
    - `canvas_app.py`: Client-side canvas application
    - `commands.py`: Client-side command handling
    - `integration_tests.py`: Integration tests
    - `unit_tests.py`: Unit tests

- `CPS2008_documentation.pdf`: Project report (video presentation included in introduction)
