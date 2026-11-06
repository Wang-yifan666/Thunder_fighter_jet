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

# ============ Windows 资源：可执行文件图标 + 版本信息 ============
# 图标 assets/icons/app.ico 为九档尺寸（16~256）的 ICO；
# 版本信息由 src/app.rc.in 配置生成，版本号取自上面的 project(... VERSION ...)。
if(WIN32)
    enable_language(RC)

    set(THUNDER_ICON_FILE
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/app.ico")

    # 为一个可执行目标挂上图标与版本资源。
    # exe_filename 写入 VERSIONINFO 的 OriginalFilename 字段。
    function(thunder_add_win_resource target exe_filename)
        if(NOT EXISTS "${THUNDER_ICON_FILE}")
            message(WARNING "图标缺失：${THUNDER_ICON_FILE}，跳过版本资源")
            return()
        endif()

        set(THUNDER_EXE_FILENAME "${exe_filename}")
        set(THUNDER_INTERNAL_NAME "${target}")

        set(_dir "${CMAKE_CURRENT_BINARY_DIR}/generated")
        file(MAKE_DIRECTORY "${_dir}")
        set(_rc "${_dir}/${target}_resource.rc")
        configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/app.rc.in"
                       "${_rc}" @ONLY)
        target_sources(${target} PRIVATE "${_rc}")
    endfunction()
else()
    function(thunder_add_win_resource target exe_filename)
    endfunction()
endif()

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
    src/io/score_path.cpp
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

thunder_add_win_resource(Thunder_fighter_jet "Thunder_fighter_jet.exe")

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
    thunder_add_win_resource(Thunder_fighter_jet_static
                             "Thunder_fighter_jet_static.exe")
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
option(THUNDER_SDL_CONSOLE "Build the SDL frontend as a console app (for debugging)" OFF)

if(THUNDER_BUILD_SDL)
    find_package(SDL3 CONFIG QUIET)
    find_package(SDL3_ttf CONFIG QUIET)

    if(SDL3_FOUND)
        add_executable(Thunder_fighter_jet_sdl
            src/sdl/main_sdl.cpp
            src/sdl/sdl_input.cpp
            src/sdl/sdl_render.cpp
            src/sdl/sdl_font.cpp
            src/sdl/bitmap_font.cpp
        )

        target_include_directories(Thunder_fighter_jet_sdl PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src/sdl
        )

        thunder_add_win_resource(Thunder_fighter_jet_sdl
                                 "Thunder_fighter_jet_sdl.exe")

        # 把精灵的烘焙数据嵌入可执行文件（与字体同一套机制）。
        # 素材改动后重跑：python tools/bake_sprites.py
        # 格式：<文件名>|<C++ 符号名>
        set(_sprite_specs
            "player_32x32|kPlayerPixels"
            "enemy_24x16|kEnemyPixels"
            "health_24x24|kHealthPixels"
            "ammo_24x24|kAmmoPixels"
        )
        file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated")
        foreach(_spec IN LISTS _sprite_specs)
            string(REPLACE "|" ";" _parts "${_spec}")
            list(GET _parts 0 _file)
            list(GET _parts 1 _symbol)
            set(_rgba "${CMAKE_CURRENT_SOURCE_DIR}/assets/sprites/${_file}.rgba")
            set(_cpp "${CMAKE_CURRENT_BINARY_DIR}/generated/${_file}_data.cpp")

            if(EXISTS "${_rgba}")
                file(READ "${_rgba}" _hex HEX)
                string(LENGTH "${_hex}" _hex_len)
                math(EXPR _byte_count "${_hex_len} / 2")
                string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1,"
                       _bytes "${_hex}")
                file(WRITE "${_cpp}"
                    "#include <cstddef>\n"
                    "#include \"sprite_data.h\"\n"
                    "namespace SpriteData\n{\n"
                    "const unsigned char ${_symbol}[] = {${_bytes}};\n"
                    "const std::size_t ${_symbol}Size = ${_byte_count}u;\n"
                    "} // namespace SpriteData\n")
            else()
                # 缺数据时给出空占位，保持可链接；绘制端会回退到色块精灵
                message(STATUS
                    "sprite data missing: assets/sprites/${_file}.rgba "
                    "(SDL falls back to colour blocks)")
                file(WRITE "${_cpp}"
                    "#include <cstddef>\n"
                    "#include \"sprite_data.h\"\n"
                    "namespace SpriteData\n{\n"
                    "const unsigned char ${_symbol}[] = {0};\n"
                    "const std::size_t ${_symbol}Size = 0u;\n"
                    "} // namespace SpriteData\n")
            endif()

            target_sources(Thunder_fighter_jet_sdl PRIVATE "${_cpp}")
        endforeach()

        target_link_libraries(Thunder_fighter_jet_sdl PRIVATE
            thunder_logic
            thunder_io
            SDL3::SDL3
        )

        if(SDL3_ttf_FOUND)
            target_compile_definitions(Thunder_fighter_jet_sdl PRIVATE
                THUNDER_HAVE_SDL3_TTF=1)
            target_link_libraries(Thunder_fighter_jet_sdl PRIVATE
                SDL3_ttf::SDL3_ttf)

            # 把 Ubuntu Mono 嵌入可执行文件：发布时只需要一个 exe。
            set(_font_file
                "${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/UbuntuMono-Regular.ttf")
            if(EXISTS "${_font_file}")
                file(READ "${_font_file}" _font_hex HEX)
                string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1,"
                       _font_bytes "${_font_hex}")
                set(_font_cpp
                    "${CMAKE_CURRENT_BINARY_DIR}/generated/ubuntu_mono_font_data.cpp")
                file(MAKE_DIRECTORY
                    "${CMAKE_CURRENT_BINARY_DIR}/generated")
                file(WRITE "${_font_cpp}"
                    "#include <cstddef>\n"
                    "#include \"font_data.h\"\n"
                    "namespace SdlFontData\n{\n"
                    "const unsigned char kUbuntuMonoRegular[] = {${_font_bytes}};\n"
                    "const std::size_t kUbuntuMonoRegularSize = sizeof(kUbuntuMonoRegular);\n"
                    "} // namespace SdlFontData\n")
                target_sources(Thunder_fighter_jet_sdl PRIVATE
                    "${_font_cpp}")
            endif()
        endif()

        target_compile_features(Thunder_fighter_jet_sdl PRIVATE cxx_std_17)
        target_compile_options(Thunder_fighter_jet_sdl PRIVATE ${THUNDER_WARNINGS})

        # 图形版默认是 GUI 子系统程序：双击启动时不应额外弹出控制台窗口。
        # 调试时可 -DTHUNDER_SDL_CONSOLE=ON 改回控制台子系统以便看日志。
        # 终端版（Thunder_fighter_jet*）保持控制台子系统不变。
        if(NOT THUNDER_SDL_CONSOLE)
            set_target_properties(Thunder_fighter_jet_sdl PROPERTIES
                WIN32_EXECUTABLE ON
            )
        endif()

        if(MINGW)
            # -static：连 libwinpthread 一起静态链接，发布只需一个 exe
            target_link_options(Thunder_fighter_jet_sdl PRIVATE
                -static-libgcc -static-libstdc++ -static)
        endif()

        install(TARGETS Thunder_fighter_jet_sdl
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

        # 字体资源随可执行文件一起部署（运行时从 assets/fonts 读取）
        add_custom_command(TARGET Thunder_fighter_jet_sdl POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts
                $<TARGET_FILE_DIR:Thunder_fighter_jet_sdl>/assets/fonts
        )
        install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts
            DESTINATION ${CMAKE_INSTALL_BINDIR}/assets)

        if(SDL3_ttf_FOUND)
            message(STATUS "SDL3 frontend enabled (SDL3 ${SDL3_VERSION}, "
                           "SDL3_ttf ${SDL3_ttf_VERSION})")
        else()
            message(STATUS "SDL3 frontend enabled (SDL3 ${SDL3_VERSION}); "
                           "SDL3_ttf not found: built-in bitmap font will be used")
        endif()
    else()
        message(STATUS "SDL3 not found: SDL frontend skipped "
                       "(configure with -DCMAKE_PREFIX_PATH=<SDL3 install>)")
    endif()
endif()

# tests (B0: only platform-independent units are covered yet)
if(THUNDER_BUILD_TESTS)
    add_subdirectory(tests)
endif()