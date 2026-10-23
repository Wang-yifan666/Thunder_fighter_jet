# B0: test harness bootstrap.
# Only platform-independent code is covered for now (HighScore).
# From B2 on, these tests will link the extracted thunder_logic library instead.

find_package(GTest CONFIG REQUIRED)

add_executable(thunder_logic_tests
    highscore_test.cpp
    ${CMAKE_SOURCE_DIR}/src/HighScore.cpp
)

target_include_directories(thunder_logic_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/inc
)

target_compile_features(thunder_logic_tests PRIVATE cxx_std_17)

target_link_libraries(thunder_logic_tests PRIVATE GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(thunder_logic_tests DISCOVERY_TIMEOUT 60)
