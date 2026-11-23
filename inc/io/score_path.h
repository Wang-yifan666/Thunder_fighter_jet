#pragma once

#include <string>

namespace thunder
{

/// 解析排行榜文件路径，按下列顺序决定：
///
/// 1. `NAIDAN_DATA_DIR` 环境变量（显式指定数据目录，测试与高级用户用）；
/// 2. exe 同目录**可写** → 用 exe 同目录的 `highscore.txt`（便携模式）；
/// 3. exe 上一级存在 `highscore.txt` → 兼容旧布局（早期从 build/ 启动时写在仓库根）；
/// 4. 以上都不满足（例如装进 Program Files）→ 用户数据目录：
///    `%APPDATA%\Naidan\highscore.txt`（不存在会自动创建）。
///
/// 之所以要判断可写性：安装到 `Program Files` 后那里不允许普通用户写入，
/// 直接往 exe 旁边写会静默失败（或被 UAC 虚拟化重定向到别处），
/// 表现为"成绩看起来保存了，重新打开却是空的"。
///
/// executable_dir 需要以路径分隔符结尾。
std::string ResolveScorePath(const std::string& executable_dir);

/// 设置文件（教程进度、当前装扮）走和排行榜完全相同的路径规则。
std::string ResolveSettingsPath(const std::string& executable_dir);

} // namespace thunder
