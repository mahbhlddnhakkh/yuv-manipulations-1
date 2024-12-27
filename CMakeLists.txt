CMAKE_MINIMUM_REQUIRED(VERSION 3.15)

project(YUVExp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL3 REQUIRED)
add_executable(${PROJECT_NAME} main.cpp MyYUV.cpp YUVCompressor.cpp YUVConverter.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE ${SDL3_LIBRARIES})