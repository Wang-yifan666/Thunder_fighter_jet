include(FetchContent)

#下载 GoogleTest
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(unit_tests test_main.cpp)

target_link_libraries(unit_tests PRIVATE GTest::gtest_main)

target_link_options(unit_tests PRIVATE "-static")

include(GoogleTest)
gtest_discover_tests(unit_tests)

