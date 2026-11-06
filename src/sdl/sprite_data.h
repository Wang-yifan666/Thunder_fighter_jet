#pragma once

#include <cstddef>

/// 构建时由 CMake 生成并嵌入的精灵数据（见 CMakeLists.txt 中的生成步骤）。
/// 数据来源：assets/sprites/*.png -> tools/bake_sprites.py -> *.rgba。
/// 格式：逐像素 R,G,B,A（直通 alpha），行优先，无文件头。
/// 数据缺失时 Size 为 0，绘制端据此回退到色块精灵。
namespace SpriteData
{

/// 玩家：32x32（命中框 3x1 格，精灵底边对齐命中框底边、向上悬垂）
extern const unsigned char kPlayerPixels[];
extern const std::size_t kPlayerPixelsSize;

/// 敌机：24x16（与 3x2 格命中框等大）
extern const unsigned char kEnemyPixels[];
extern const std::size_t kEnemyPixelsSize;

/// 血包：24x24（与 3x3 格判定等大）
extern const unsigned char kHealthPixels[];
extern const std::size_t kHealthPixelsSize;

/// 弹药包：24x24（与 3x3 格判定等大）
extern const unsigned char kAmmoPixels[];
extern const std::size_t kAmmoPixelsSize;

} // namespace SpriteData
