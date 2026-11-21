cmake_minimum_required(VERSION 3.20)

if (DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

# 版本号同时提供给 Windows 资源（app.rc.in），改版本只改这里。
project(Naiden VERSION 1.7 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(THUNDER_BUILD_TESTS "构建单元测试（需要 vcpkg 的 gtest）" ON)
option(THUNDER_SDL_CONSOLE "以控制台子系统构建（调试用，便于看日志）" OFF)

if(MSVC)
    set(THUNDER_WARNINGS /W4)
else()
    set(THUNDER_WARNINGS -Wall -Wextra)
endif()

enable_testing()

# ============ Windows 资源：图标 + 版本信息 ============
# 图标 assets/icons/app.ico 为九档尺寸（16~256）的 ICO；
# 版本信息由 src/app.rc.in 配置生成，版本号取自上面的 project(... VERSION ...)。
if(WIN32)
    enable_language(RC)

    set(THUNDER_ICON_FILE
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/app.ico")

    # 为可执行目标挂图标与版本资源；exe_filename 写入 OriginalFilename 字段。
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

# ============ thunder_logic：平台无关的游戏世界 ============
# 不允许链接 SDL / 出现 windows.h 等平台头（tests/check_isolation.cmake 强制）。
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

# ============ thunder_io：文件存取（逻辑层不得依赖） ============
add_library(thunder_io STATIC
    src/io/file_score_store.cpp
    src/io/score_path.cpp
)

target_include_directories(thunder_io PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

target_compile_features(thunder_io PUBLIC cxx_std_17)
target_compile_options(thunder_io PRIVATE ${THUNDER_WARNINGS})

# ============ 外部依赖 ============
find_package(SDL3 CONFIG REQUIRED)
find_package(SDL3_ttf CONFIG QUIET)

# ============ Naidan：SDL3 图形前端（唯一的可执行文件） ============
add_executable(Naidan
    src/sdl/main_sdl.cpp
    src/sdl/sdl_input.cpp
    src/sdl/sdl_render.cpp
    src/sdl/sdl_font.cpp
    src/sdl/bitmap_font.cpp
)

target_include_directories(Naidan PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sdl
)

thunder_add_win_resource(Naidan "Naidan.exe")

target_link_libraries(Naidan PRIVATE
    thunder_logic
    thunder_io
    SDL3::SDL3
)

# 把精灵数据嵌入可执行文件。素材改动后重跑：python tools/bake_sprites.py
# 格式：<文件名>|<C++ 符号名>
set(_sprite_specs
    "player_32x32|kPlayerPixels"
    "enemy_24x16|kEnemyPixels"
    "elite_32x24|kElitePixels"
    "health_24x24|kHealthPixels"
    "ammo_24x24|kAmmoPixels"
    "shield_pack_24x24|kShieldPackPixels"
    "firepower_24x24|kFirepowerPixels"
    "shield_bubble_40x40|kShieldBubblePixels"
    "shield_bubble_big_56x56|kShieldBubbleBigPixels"
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
        string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," _bytes "${_hex}")
        file(WRITE "${_cpp}"
            "#include <cstddef>\n"
            "#include \"sprite_data.h\"\n"
            "namespace SpriteData\n{\n"
            "const unsigned char ${_symbol}[] = {${_bytes}};\n"
            "const std::size_t ${_symbol}Size = ${_byte_count}u;\n"
            "} // namespace SpriteData\n")
    else()
        # 缺数据时给出空占位，保持可链接；绘制端会回退到色块精灵
        message(STATUS "sprite data missing: assets/sprites/${_file}.rgba "
                       "(falls back to colour blocks)")
        file(WRITE "${_cpp}"
            "#include <cstddef>\n"
            "#include \"sprite_data.h\"\n"
            "namespace SpriteData\n{\n"
            "const unsigned char ${_symbol}[] = {0};\n"
            "const std::size_t ${_symbol}Size = 0u;\n"
            "} // namespace SpriteData\n")
    endif()

    target_sources(Naidan PRIVATE "${_cpp}")
endforeach()

if(SDL3_ttf_FOUND)
    target_compile_definitions(Naidan PRIVATE THUNDER_HAVE_SDL3_TTF=1)
    target_link_libraries(Naidan PRIVATE SDL3_ttf::SDL3_ttf)

    # 把 Ubuntu Mono 嵌入可执行文件：发布时只需要一个 exe。
    set(_font_file
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/UbuntuMono-Regular.ttf")
    if(EXISTS "${_font_file}")
        file(READ "${_font_file}" _font_hex HEX)
        string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," _font_bytes "${_font_hex}")
        set(_font_cpp
            "${CMAKE_CURRENT_BINARY_DIR}/generated/ubuntu_mono_font_data.cpp")
        file(WRITE "${_font_cpp}"
            "#include <cstddef>\n"
            "#include \"font_data.h\"\n"
            "namespace SdlFontData\n{\n"
            "const unsigned char kUbuntuMonoRegular[] = {${_font_bytes}};\n"
            "const std::size_t kUbuntuMonoRegularSize = sizeof(kUbuntuMonoRegular);\n"
            "} // namespace SdlFontData\n")
        target_sources(Naidan PRIVATE "${_font_cpp}")
    else()
        message(STATUS "font missing: ${_font_file} (built-in bitmap font will be used)")
    endif()
endif()

target_compile_features(Naidan PRIVATE cxx_std_17)
target_compile_options(Naidan PRIVATE ${THUNDER_WARNINGS})

# 默认 GUI 子系统：双击启动不弹控制台。调试时 -DTHUNDER_SDL_CONSOLE=ON。
if(NOT THUNDER_SDL_CONSOLE)
    set_target_properties(Naidan PROPERTIES WIN32_EXECUTABLE ON)
endif()

# 单文件发布：运行库、libwinpthread 一并静态链接
if(MINGW)
    target_link_options(Naidan PRIVATE -static-libgcc -static-libstdc++ -static)
elseif(MSVC)
    set_property(TARGET Naidan PROPERTY
                 MSVC_RUNTIME_LIBRARY "MultiThreaded")
endif()

include(GNUInstallDirs)
install(TARGETS Naidan RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# 字体内嵌进 exe，但授权文件必须随发布一起提供
install(FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/LICENSE-Ubuntu-Font-Licence-1.0.txt"
    DESTINATION "${CMAKE_INSTALL_BINDIR}/licenses"
)

if(SDL3_ttf_FOUND)
    message(STATUS "Naidan: SDL3 ${SDL3_VERSION}, SDL3_ttf ${SDL3_ttf_VERSION}, "
                   "font and sprites embedded")
else()
    message(STATUS "Naidan: SDL3 ${SDL3_VERSION}, SDL3_ttf NOT found — "
                   "built-in bitmap font will be used")
endif()

# ============ 测试 ============
if(THUNDER_BUILD_TESTS)
    add_subdirectory(tests)
endif()
