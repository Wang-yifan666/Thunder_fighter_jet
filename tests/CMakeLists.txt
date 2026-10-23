# B3：逻辑层与终端层回归测试

find_package(GTest CONFIG REQUIRED)
include(GoogleTest)

# ============ 逻辑层测试：只链接 thunder_logic / thunder_io ============
add_executable(thunder_logic_tests
    highscore_test.cpp
    world_test.cpp
    rng_test.cpp
    session_test.cpp
)

target_include_directories(thunder_logic_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(thunder_logic_tests PRIVATE cxx_std_17)

target_link_libraries(thunder_logic_tests PRIVATE
    GTest::gtest_main
    thunder_logic
    thunder_io
)

gtest_discover_tests(thunder_logic_tests DISCOVERY_TIMEOUT 60)

# ============ 终端层测试：字符网格 / 血量字符串 ============
add_executable(thunder_terminal_tests
    golden_cell_grid_test.cpp
)

target_compile_features(thunder_terminal_tests PRIVATE cxx_std_17)

target_link_libraries(thunder_terminal_tests PRIVATE
    GTest::gtest_main
    thunder_core
)

if(WIN32)
    target_link_libraries(thunder_terminal_tests PRIVATE winmm)
endif()

gtest_discover_tests(thunder_terminal_tests DISCOVERY_TIMEOUT 60)

# ============ 隔离检查：逻辑层不得出现平台依赖 ============
add_test(NAME isolation.no_platform_in_logic
    COMMAND ${CMAKE_COMMAND}
        -DLOGIC_INC_DIR=${CMAKE_SOURCE_DIR}/inc/game
        -DLOGIC_SRC_DIR=${CMAKE_SOURCE_DIR}/src/game
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check_isolation.cmake
)
