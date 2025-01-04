CMAKE_MINIMUM_REQUIRED(VERSION 3.15)

project(YUVExp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL3 REQUIRED)
find_package(OpenMP REQUIRED)
add_executable(${PROJECT_NAME} main.cpp MyYUV.cpp YUVCompressor.cpp YUVConverter.cpp MyUtil.cpp)

# for assert for RelWithDebInfo
# https://stackoverflow.com/questions/22140520/how-to-enable-assert-in-cmake-release-mode/42119938#42119938
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  string( REPLACE "-DNDEBUG" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
endif()
if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  string( REPLACE "/DNDEBUG" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE ${SDL3_LIBRARIES} PRIVATE OpenMP::OpenMP_CXX)
