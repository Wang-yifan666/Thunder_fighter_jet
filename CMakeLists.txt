cmake_minimum_required(VERSION 3.14)

if (DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

project(Thunder_fighter_jet VERSION 1.6 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(THUNDER_STATIC_MINGW "When using MinGW, link the executable statically" OFF)
option(BUILD_ADDITIONAL_STATIC_EXE "Also build a statically linked exe target alongside the dynamic one" ON)
option(THUNDER_BUILD_TESTS "Build unit tests (requires GoogleTest from vcpkg)" ON)

# 新代码（逻辑层 / IO 层 / SDL 前端 / 测试）开启警告；终端历史代码暂不开启
if(MSVC)
    set(THUNDER_WARNINGS /W4)
else()
    set(THUNDER_WARNINGS -Wall -Wextra)
endif()

enable_testing()

# ftxui
find_package(ftxui CONFIG REQUIRED)

# ============ thunder_logic: 平台无关的游戏世界 ============
# 不允许链接 ftxui / SDL / winmm，也不允许出现 windows.h 等平台头。
add_library(thunder_logic STATIC
    src/game/game_world.cpp
    src/game/game_world_enemy.cpp
    src/game/game_world_cheats.cpp
    src/game/game_session.cpp
    src/game/highscore.cpp
)

target_include_directories(thunder_logic PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

target_compile_features(thunder_logic PUBLIC cxx_std_17)
target_compile_options(thunder_logic PRIVATE ${THUNDER_WARNINGS})

# ============ thunder_io: 文件存取（逻辑层不得依赖） ============
add_library(thunder_io STATIC
    src/io/file_score_store.cpp
)

target_include_directories(thunder_io PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

target_compile_features(thunder_io PUBLIC cxx_std_17)
target_compile_options(thunder_io PRIVATE ${THUNDER_WARNINGS})

# ============ thunder_core: 终端前端（FTXUI + Win32 控制台） ============
set(THUNDER_CORE_SOURCES
    src/def.cpp
    src/cell_grid.cpp
    src/console_utils.cpp
    src/input_console.cpp
    src/render_console.cpp
)

add_library(thunder_core ${THUNDER_CORE_SOURCES})

target_include_directories(thunder_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(thunder_core PUBLIC
    thunder_logic
    thunder_io
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

# ============ SDL3 前端（可选，独立可执行文件） ============
option(THUNDER_BUILD_SDL "Build the SDL3 frontend when SDL3 is available" ON)

if(THUNDER_BUILD_SDL)
    find_package(SDL3 CONFIG QUIET)

    if(SDL3_FOUND)
        add_executable(Thunder_fighter_jet_sdl
            src/sdl/main_sdl.cpp
            src/sdl/sdl_input.cpp
            src/sdl/sdl_render.cpp
            src/sdl/bitmap_font.cpp
        )

        target_include_directories(Thunder_fighter_jet_sdl PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src/sdl
        )

        target_link_libraries(Thunder_fighter_jet_sdl PRIVATE
            thunder_logic
            thunder_io
            SDL3::SDL3
        )

        target_compile_features(Thunder_fighter_jet_sdl PRIVATE cxx_std_17)
        target_compile_options(Thunder_fighter_jet_sdl PRIVATE ${THUNDER_WARNINGS})

        if(MINGW)
            target_link_options(Thunder_fighter_jet_sdl PRIVATE
                -static-libgcc -static-libstdc++)
        endif()

        install(TARGETS Thunder_fighter_jet_sdl
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

        message(STATUS "SDL3 frontend enabled (SDL3 ${SDL3_VERSION})")
    else()
        message(STATUS "SDL3 not found: SDL frontend skipped "
                       "(configure with -DCMAKE_PREFIX_PATH=<SDL3 install>)")
    endif()
endif()

# tests (B0: only platform-independent units are covered yet)
if(THUNDER_BUILD_TESTS)
    add_subdirectory(tests)
endif()