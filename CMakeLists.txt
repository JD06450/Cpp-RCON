cmake_minimum_required(VERSION 3.23.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(Cpp-RCON VERSION 0.1.0)

add_library(Lib-Cpp-RCON SHARED
	src/libindex.cpp
)

add_executable(Exe-Cpp-RCON
	src/index.cpp
	src/libindex.cpp
)

set_target_properties(Lib-Cpp-RCON PROPERTIES
	OUTPUT_NAME "cpp-rcon"
)

set_target_properties(Exe-Cpp-RCON PROPERTIES
	OUTPUT_NAME "open-rcon"
)

if(CMAKE_BUILD_TYPE MATCHES "Debug")
	add_compile_definitions(DEBUG)
else()
	add_compile_definitions(NDEBUG)
endif()

target_include_directories(Lib-Cpp-RCON PUBLIC include)
target_include_directories(Exe-Cpp-RCON PRIVATE include)