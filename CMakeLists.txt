cmake_minimum_required(VERSION 3.10)
project(reck_project)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# add_compile_options("-DDEBUG")
add_compile_options("-O3" "-ggdb" "-Wall" "-Wextra")
include_directories(include)

add_subdirectory(src)

install(TARGETS reck
        EXPORT reck-targets
        LIBRARY DESTINATION lib
        PUBLIC_HEADER DESTINATION include)

install(DIRECTORY include
        DESTINATION include)

enable_testing()
add_subdirectory(test)