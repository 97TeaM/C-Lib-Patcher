cmake_minimum_required(VERSION 3.10)

project(Injector)

set(CMAKE_CXX_STANDARD 11) # Set the C++ standard to C++11

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -target armv7a-linux-androideabi19 -static-libstdc++")

# Source files
set(SRC_FILES injector.cpp)

# Output executable
add_executable(injector ${SRC_FILES})

# Specify executable output path
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)

# Clean target
add_custom_target(clean-all
    COMMAND ${CMAKE_COMMAND} -P clean-all.cmake
)

# Clean-all script
file(WRITE ${CMAKE_BINARY_DIR}/clean-all.cmake
    "FILE(REMOVE_RECURSE ${CMAKE_SOURCE_DIR}/bin)"
)
