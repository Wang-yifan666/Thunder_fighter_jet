add_executable(tests
    test_main.cpp
)

target_link_libraries(tests PRIVATE
    thunder_core
    GTest::gtest_main
)

if(MINGW)
    target_link_options(tests PRIVATE -static)
endif()

include(GoogleTest)
gtest_discover_tests(tests)
