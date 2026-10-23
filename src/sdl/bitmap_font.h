#pragma once

#include <SDL3/SDL.h>

#include <cstdint>
#include <string>

namespace BitmapFont
{

/// 5x7 ASCII 点阵字体，避免引入 SDL_ttf / freetype 依赖。
/// 只覆盖游戏 HUD 需要的字符：大写字母、数字与少量符号；小写按大写绘制。
void DrawText(SDL_Renderer* renderer, int x, int y, int scale,
              const std::string& text, SDL_Color color);

/// 文本像素宽度（每字符 6*scale 像素，含 1 像素间距）
int TextWidth(const std::string& text, int scale);

} // namespace BitmapFont
