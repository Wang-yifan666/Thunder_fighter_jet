#pragma once

#include <SDL3/SDL.h>

#include "game/session_snapshot.h"

namespace SdlRender
{

struct Layout
{
	int window_width = 832;  // 8px 边距 * 2 + 102 列 * 8px
	int window_height = 496; // 64px HUD + 48 行 * 8px 棋盘 + 48px 底部提示
	int cell = 8;
	int grid_x = 8;
	int grid_y = 64;
};

/// 按 SessionSnapshot 绘制一帧（菜单 / 难度 / 排行榜 / 游戏 / 结算）。
/// seconds 为程序运行秒数，仅用于纯视觉动画（星空滚动、无敌闪烁）。
void Draw(SDL_Renderer* renderer, const thunder::SessionSnapshot& snapshot, int fps,
          double seconds = 0.0);

} // namespace SdlRender
