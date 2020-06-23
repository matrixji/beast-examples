cmake_minimum_required(VERSION 3.12)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(beast_examples)

add_library(democlient
        src/HttpClient.cpp
        src/democlient.cpp
        src/Utils.cpp)

add_executable(testclient src/testclient.c)
target_link_libraries(testclient democlient pthread)


add_executable(beast_server
        src/main.cpp
        src/HttpListener.cpp
        src/HttpSession.cpp
        src/WebsocketSession.cpp
        src/StaticRequestHandler.cpp
        src/Utils.cpp
        src/JsonApiRequestHandler.cpp
        src/HttpUriRouter.cpp
        src/PicturePreviewHandler.cpp
        src/WebsocketHandler.cpp)

target_link_libraries(beast_server PUBLIC pthread)

find_package(Boost COMPONENTS REQUIRED)
if(Boost_FOUND)
    include_directories(PRIVATE ${Boost_INCLUDE_DIRS} third-part/include)
    MESSAGE(STATUS "Boost_INCLUDE_DIRS = ${Boost_INCLUDE_DIRS}.")
    MESSAGE(STATUS "Boost_LIBRARIES = ${Boost_LIBRARIES}.")
    MESSAGE(STATUS "Boost_LIB_VERSION = ${Boost_LIB_VERSION}.")
endif()

# c++11
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11")
else()
    message(FATAL_ERROR "${CMAKE_CXX_COMPILER} has no c++11 support.")
endif()

# extra flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
