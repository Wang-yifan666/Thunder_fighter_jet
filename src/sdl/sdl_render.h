#pragma once

#include <SDL3/SDL.h>

#include "game/session_snapshot.h"
#include "view/scene_layout.h"

namespace SdlRender
{

/// 布局本身是平台无关的，定义在 inc/view/scene_layout.h；
/// 这里保留别名，前端代码仍可按原来的名字使用。
using Layout = View::Layout;

/// 按 SessionSnapshot 绘制一帧（菜单 / 难度 / 排行榜 / 游戏 / 结算）。
/// seconds 为程序运行秒数，仅用于纯视觉动画（星空滚动、无敌闪烁）。
void Draw(SDL_Renderer* renderer, const thunder::SessionSnapshot& snapshot, int fps,
          double seconds = 0.0);

} // namespace SdlRender
