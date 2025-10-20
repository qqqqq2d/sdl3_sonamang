cmake_minimum_required(VERSION 4.0)
project(text_test)

set(CMAKE_CXX_STANDARD 20)

find_package(SDL3 REQUIRED)
find_package(SDL3_ttf REQUIRED)

add_executable(text_test main.cpp)

target_link_libraries(text_test PRIVATE SDL3::SDL3 SDL3_ttf::SDL3_ttf)

target_include_directories(${PROJECT_NAME} PRIVATE
        ${SDL3_INCLUDE_DIRS}
)