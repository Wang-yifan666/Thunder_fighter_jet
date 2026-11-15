#pragma once

/// 产品信息：名称与版本号只在这里定义，前端、资源脚本、文档都从这里取。
/// 版本号同时由 CMakeLists.txt 的 project(... VERSION ...) 提供给 Windows 资源，
/// 两处必须保持一致（改版本时一起改）。
namespace GameInfo
{

constexpr const char* kProductNameZh = "奶蛋的火力不足恐惧症";
constexpr const char* kProductNameEn = "Naidan's Fear of Insufficient Firepower";
constexpr const char* kVersion = "1.7";

/// 带 v 前缀的版本串，用于窗口标题等展示场合。
constexpr const char* kVersionTag = "v1.7";

} // namespace GameInfo
