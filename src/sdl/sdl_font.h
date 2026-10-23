#pragma once

#include <SDL3/SDL.h>

#include <string>

namespace SdlFont
{

/// 加载 assets/fonts/UbuntuMono-Regular.ttf。
/// 未编译 SDL3_ttf 或字体文件缺失时返回 false，之后统一回退到内置点阵字体。
bool Init();

void Shutdown();

bool UsingTrueType();

/// scale 与内置点阵保持一致（1..4）；TTF 模式下映射为像素字号（12/18/24/30 px）。
void DrawText(SDL_Renderer* renderer, int x, int y, int scale,
              const std::string& text, SDL_Color color);

int TextWidth(const std::string& text, int scale);

} // namespace SdlFont
