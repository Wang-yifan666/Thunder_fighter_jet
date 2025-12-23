cmake_minimum_required(VERSION 3.14)

if (DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

project(Thunder_fighter_jet LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#ftxui
find_package(ftxui CONFIG REQUIRED)

#GoogleTest
include(FetchContent)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

#thunder_core
add_library(thunder_core
    src/def.cpp
    src/enemy.cpp
    src/cheats.cpp
)

target_include_directories(thunder_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(thunder_core PUBLIC
    ftxui::dom
    ftxui::screen
    ftxui::component
)

#主程序
add_executable(Thunder_fighter_jet
    src/main.cpp
)

target_link_libraries(Thunder_fighter_jet PRIVATE
    thunder_core
    winmm
)

if(MINGW)
  target_link_options(Thunder_fighter_jet PRIVATE -static)
endif()

#tests
enable_testing()
add_subdirectory(tests)
