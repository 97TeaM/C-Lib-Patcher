cmake_minimum_required(VERSION 3.10)

project(Injector)

set(CMAKE_CXX_STANDARD 17)
set(SRC_FILES injector.cpp)

add_executable(injector ${SRC_FILES})

target_compile_options(injector PRIVATE -Wall -march=armv7-a)
target_link_options(injector PRIVATE --static-libstdc++ --fix-cortex-a8)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)

add_custom_target(clean-all
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_SOURCE_DIR}/bin
)
