#pragma once

#include <string>

namespace thunder
{

/// 解析排行榜文件路径：
/// 1. exe 同目录已有 highscore.txt → 用它；
/// 2. 否则 exe 上一级（兼容重构前的 ../highscore.txt）→ 用它；
/// 3. 都没有 → 返回 exe 同目录下的 highscore.txt（首次运行会创建）。
/// executable_dir 需要以路径分隔符结尾。
std::string ResolveScorePath(const std::string& executable_dir);

} // namespace thunder
