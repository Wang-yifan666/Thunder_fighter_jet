# B0：测试链路骨架，目前只覆盖与平台无关的 HighScore。
# B3 起追加 GameWorld / CellGrid 的回归用例。

find_package(GTest CONFIG REQUIRED)

add_executable(thunder_logic_tests
    highscore_test.cpp
)

target_compile_features(thunder_logic_tests PRIVATE cxx_std_17)

target_link_libraries(thunder_logic_tests PRIVATE
    GTest::gtest_main
    thunder_io
)

include(GoogleTest)
gtest_discover_tests(thunder_logic_tests DISCOVERY_TIMEOUT 60)
