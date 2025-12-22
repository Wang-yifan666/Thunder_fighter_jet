cmake_minimum_required(VERSION 3.14)

if (DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

project(Thunder_fighter_jet LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ftxui CONFIG REQUIRED)

add_executable(Thunder_fighter_jet
    src/main.cpp
    src/def.cpp
    src/enemy.cpp
    src/cheats.cpp
)

target_link_libraries(Thunder_fighter_jet PRIVATE
    ftxui::dom
    ftxui::screen
    ftxui::component
    winmm
)

target_link_options(Thunder_fighter_jet PRIVATE "-static")
 
enable_testing() 
add_subdirectory(tests)
