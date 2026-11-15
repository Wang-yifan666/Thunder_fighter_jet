# 逻辑层回归测试（只链接 thunder_logic / thunder_io）

find_package(GTest CONFIG REQUIRED)
include(GoogleTest)

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
target_compile_options(thunder_logic_tests PRIVATE ${THUNDER_WARNINGS})

target_link_libraries(thunder_logic_tests PRIVATE
    GTest::gtest_main
    thunder_logic
    thunder_io
)

gtest_discover_tests(thunder_logic_tests DISCOVERY_TIMEOUT 60)

# ============ 隔离检查：逻辑层不得出现平台依赖 ============
add_test(NAME isolation.no_platform_in_logic
    COMMAND ${CMAKE_COMMAND}
        -DLOGIC_INC_DIR=${CMAKE_SOURCE_DIR}/inc/game
        -DLOGIC_SRC_DIR=${CMAKE_SOURCE_DIR}/src/game
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check_isolation.cmake
)
