cmake_minimum_required(VERSION 3.14)

if (DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

project(Thunder_fighter_jet VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_SHARED_LIBS "Build libraries as shared" OFF)
option(THUNDER_STATIC_MINGW "When using MinGW, link the executable statically" OFF)
option(BUILD_ADDITIONAL_STATIC_EXE "Also build a statically linked exe target alongside the dynamic one" ON)

# ftxui
find_package(ftxui CONFIG REQUIRED)

# sources
set(THUNDER_CORE_SOURCES
    src/def.cpp
    src/enemy.cpp
    src/cheats.cpp
    src/HighScore.cpp
    src/input_console.cpp
    src/console_utils.cpp
    src/render_console.cpp
)

add_library(thunder_core ${THUNDER_CORE_SOURCES})

target_include_directories(thunder_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(thunder_core PUBLIC
    ftxui::dom
    ftxui::screen
    ftxui::component
)

target_compile_features(thunder_core PUBLIC cxx_std_17)

# main executable
add_executable(Thunder_fighter_jet src/main.cpp)

target_link_libraries(Thunder_fighter_jet PRIVATE thunder_core)

if(WIN32)
    target_link_libraries(Thunder_fighter_jet PRIVATE winmm)
endif()

if(MINGW AND THUNDER_STATIC_MINGW)
    target_link_options(Thunder_fighter_jet PRIVATE -static)
endif()

# Optional: build an additional statically-linked executable (no external libstdc++/libgcc DLLs)
if(MINGW AND BUILD_ADDITIONAL_STATIC_EXE)
    add_executable(Thunder_fighter_jet_static src/main.cpp)
    target_link_libraries(Thunder_fighter_jet_static PRIVATE thunder_core)
    target_compile_features(Thunder_fighter_jet_static PRIVATE cxx_std_17)
    # Prefer partial static linking to avoid depending on libstdc++/libgcc DLLs
    target_link_options(Thunder_fighter_jet_static PRIVATE -static-libgcc -static-libstdc++)
    if(THUNDER_STATIC_MINGW)
        target_link_options(Thunder_fighter_jet_static PRIVATE -static)
    endif()
    install(TARGETS Thunder_fighter_jet_static RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

# Install rules (optional)
include(GNUInstallDirs)
install(TARGETS thunder_core Thunder_fighter_jet
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)