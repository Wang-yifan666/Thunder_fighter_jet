#pragma once

#include <SDL3/SDL.h>

#include "game/session_snapshot.h"

namespace SdlRender
{

struct Layout
{
	int window_width = 800;
	int window_height = 432;
	int cell = 8;
	int grid_x = 8;
	int grid_y = 64;
};

/// 按 SessionSnapshot 绘制一帧（菜单 / 难度 / 排行榜 / 游戏 / 结算）。
void Draw(SDL_Renderer* renderer, const thunder::SessionSnapshot& snapshot,
          int fps);

} // namespace SdlRender
