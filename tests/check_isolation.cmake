# 隔离检查：thunder_logic 的源码不得依赖平台 / UI / 系统时钟 / 文件 IO。
# 由 ctest 调用：cmake -DLOGIC_INC_DIR=... -DLOGIC_SRC_DIR=... -P check_isolation.cmake

file(GLOB_RECURSE logic_files
    "${LOGIC_INC_DIR}/*.h"
    "${LOGIC_SRC_DIR}/*.cpp"
)

if(NOT logic_files)
    message(FATAL_ERROR "隔离检查失败：未找到任何逻辑层源文件（检查路径参数）")
endif()

set(forbidden
    "windows[.]h"
    "ftxui"
    "SDL_"
    "GetAsyncKeyState"
    "ConsoleUtils"
    "system[(]"
    "std::cout"
    "std::cerr"
    "std::this_thread"
    "steady_clock::now"
    "std::ifstream"
    "std::ofstream"
    "fopen"
    "SleepMs[(]"
    "sleep_for"
)

foreach(file ${logic_files})
    file(READ "${file}" content)
    foreach(pattern ${forbidden})
        string(REGEX MATCH "${pattern}" hit "${content}")
        if(hit)
            message(FATAL_ERROR
                "隔离检查失败：${file} 命中禁用模式 '${pattern}'")
        endif()
    endforeach()
endforeach()

list(LENGTH logic_files file_count)
message(STATUS "隔离检查通过：${file_count} 个逻辑层文件未发现平台依赖")
