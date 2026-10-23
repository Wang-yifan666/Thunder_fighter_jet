#pragma once

#include <cstddef>

/// 构建时由 CMake 生成并嵌入的字体数据（见 CMakeLists.txt 中的生成步骤）。
namespace SdlFontData
{
extern const unsigned char kUbuntuMonoRegular[];
extern const std::size_t kUbuntuMonoRegularSize;
} // namespace SdlFontData
