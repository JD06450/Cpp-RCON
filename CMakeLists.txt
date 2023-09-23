cmake_minimum_required(VERSION 3.23.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

project(Cpp-RCON VERSION 0.2.0)

find_package(Boost REQUIRED COMPONENTS program_options)

# message("Boost libs: " ${Boost_LIBRARIES})
add_library(Lib-Cpp-RCON SHARED
	src/libindex.cpp
	src/logger.cpp
)

add_executable(Exe-Cpp-RCON
	src/index.cpp
	src/libindex.cpp
	src/logger.cpp
)

set_target_properties(Lib-Cpp-RCON PROPERTIES
	OUTPUT_NAME "cpp-rcon"
)

set_target_properties(Exe-Cpp-RCON PROPERTIES
	OUTPUT_NAME "open-rcon"
)

if (Boost_FOUND)
	target_include_directories(Exe-Cpp-RCON PRIVATE ${Boost_INCLUDE_DIRS})
	target_link_libraries(Exe-Cpp-RCON PRIVATE ${Boost_LIBRARIES})
endif()


target_include_directories(Lib-Cpp-RCON PUBLIC include)
target_include_directories(Exe-Cpp-RCON PRIVATE include)