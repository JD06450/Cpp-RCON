cmake_minimum_required(VERSION 3.23.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(Cpp-RCON VERSION 0.1.0)

add_library(Lib-Cpp-RCON SHARED
	src/libindex.cpp
)

add_executable(Exe-Cpp-RCON
	src/index.cpp
)

set_target_properties(Lib-Cpp-RCON PROPERTIES
	OUTPUT_NAME "cpp-rcon"
)

set_target_properties(Exe-Cpp-RCON PROPERTIES
	OUTPUT_NAME "open-rcon"
)

target_include_directories(Lib-Cpp-RCON PUBLIC include)
target_include_directories(Exe-Cpp-RCON PRIVATE include)