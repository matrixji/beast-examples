cmake_minimum_required(VERSION 3.12)

set(CMAKE_CPP_STANDARD c++14)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(beast_test)

add_executable(beast_test
        src/main.cpp
        src/HttpListener.cpp
        src/HttpSession.cpp
        src/WebSocketSession.cpp
        src/StaticRequestHandler.cpp
        src/Utils.cpp
        src/JsonApiRequestHandler.cpp
        src/HttpUriRouter.cpp
        src/PicturePreviewHandler.cpp)

target_include_directories(beast_test PRIVATE third-part/include)
target_link_libraries(beast_test PUBLIC pthread)