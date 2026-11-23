#pragma once

#include <cstddef>

/// 构建时由 CMake 生成并嵌入的字体数据（见 CMakeLists.txt 中的生成步骤）。
namespace SdlFontData
{
extern const unsigned char kUbuntuMonoRegular[];
extern const std::size_t kUbuntuMonoRegularSize;

/// 中文子集（Noto Sans SC，只含 inc/game/game_text.h 用到的字），
/// 作为 Ubuntu Mono 的 fallback 字体挂上去：ASCII 走等宽字体，汉字自动落到这里。
extern const unsigned char kCjkSubset[];
extern const std::size_t kCjkSubsetSize;
} // namespace SdlFontData
