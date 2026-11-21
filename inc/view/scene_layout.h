#pragma once

/// 平台无关的场景布局与精灵元数据。
///
/// 这一层只负责"格子坐标 <-> 像素矩形"的换算、精灵尺寸和配色常量，
/// 不包含任何平台相关的调用，因此可以被其它前端（Android 等）复用。
/// 真正画到屏幕上的动作仍由各平台后端完成。
///
/// 之所以把 Layout 做成"带字段的结构体"而不是一堆常量：将来移植到手机上时，
/// 窗口尺寸和格子大小需要按屏幕自适应，实例化的 Layout 才能表达这种差异。

#include "game/game_config.h"

namespace View
{

/// 逻辑分辨率与网格。默认值对应 832x496 的桌面窗口。
struct Layout
{
	int window_width = 832;  // 8px 边距 * 2 + 102 列 * 8px
	int window_height = 496; // 64px HUD + 48 行 * 8px 棋盘 + 48px 底部提示
	int cell = 8;
	int grid_x = 8;
	int grid_y = 64;

	int GridPixelWidth() const
	{
		return GameConfig::kScreenWidth * cell;
	}

	int GridPixelHeight() const
	{
		return GameConfig::kScreenHeight * cell;
	}
};

// ============ 精灵像素尺寸 ============
/// 玩家精灵：32x32（4 格宽）。命中框仍是 3x1 格，底边对齐后向上悬垂，
/// 视觉比判定大一圈，对玩家更宽容。
inline constexpr int kPlayerSpriteSize = 32;

/// 敌机精灵：24x16（3x2 格），与命中框严格等大。
inline constexpr int kEnemySpriteWidth = 24;
inline constexpr int kEnemySpriteHeight = 16;

/// 精英精灵：32x24（4x3 格），与命中框严格等大。
inline constexpr int kEliteSpriteWidth = 32;
inline constexpr int kEliteSpriteHeight = 24;

/// 掉落物精灵：24x24（3x3 格），与拾取判定等大。
inline constexpr int kPickupSpriteSize = 24;

/// 护盾泡：小怪 32x32 罩住 3x2；精英 4x3 与玩家 32x32 精灵共用 48x48。
inline constexpr int kShieldBubbleSize = 32;
inline constexpr int kShieldBubbleBigSize = 48;

// ============ 网格换算 ============
struct Rect
{
	float x = 0.0f;
	float y = 0.0f;
	float w = 0.0f;
	float h = 0.0f;
};

/// 某个格子左上角在窗口中的像素坐标。
inline float CellX(const Layout& layout, int cell_x)
{
	return static_cast<float>(layout.grid_x) + static_cast<float>(cell_x * layout.cell);
}

inline float CellY(const Layout& layout, int cell_y)
{
	return static_cast<float>(layout.grid_y) + static_cast<float>(cell_y * layout.cell);
}

/// 任意格子矩形。
inline Rect CellRect(const Layout& layout, int cell_x, int cell_y, int cell_w, int cell_h)
{
	const float cell = static_cast<float>(layout.cell);
	return Rect{CellX(layout, cell_x), CellY(layout, cell_y), static_cast<float>(cell_w) * cell,
	            static_cast<float>(cell_h) * cell};
}

/// 玩家精灵：水平以命中框居中，竖直底边对齐命中框底边。
inline Rect PlayerRect(const Layout& layout, int player_x, int player_y)
{
	const float cell = static_cast<float>(layout.cell);
	const float size = static_cast<float>(kPlayerSpriteSize);
	const float left = CellX(layout, player_x)
	    + static_cast<float>(GameConfig::kPlayerWidth) * cell * 0.5f - size * 0.5f;
	const float bottom = CellY(layout, player_y) + cell;
	return Rect{left, bottom - size, size, size};
}

/// 敌机精灵：与命中框完全重合。
inline Rect EnemyRect(const Layout& layout, int enemy_x, int enemy_y, int enemy_w, int enemy_h)
{
	return CellRect(layout, enemy_x, enemy_y, enemy_w, enemy_h);
}

/// 掉落物精灵：与拾取判定重合，bob 是纯视觉的上下浮动像素。
inline Rect PickupRect(const Layout& layout, int pickup_x, int pickup_y, int pickup_w, int pickup_h,
                       float bob = 0.0f)
{
	const float cell = static_cast<float>(layout.cell);
	return Rect{CellX(layout, pickup_x), CellY(layout, pickup_y) + bob,
	            static_cast<float>(pickup_w) * cell, static_cast<float>(pickup_h) * cell};
}

/// 子弹：以格子中心为锚点居中绘制。
inline Rect BulletRect(const Layout& layout, int bullet_x, int bullet_y, float width, float height)
{
	const float cell = static_cast<float>(layout.cell);
	const float cx = CellX(layout, bullet_x) + cell * 0.5f;
	const float cy = CellY(layout, bullet_y) + cell * 0.5f;
	return Rect{cx - width * 0.5f, cy - height * 0.5f, width, height};
}

/// 以某个像素点为中心的正方形（护盾泡用：比本体大一整圈）。
inline Rect CenteredSquare(float center_x, float center_y, int size)
{
	const float s = static_cast<float>(size);
	return Rect{center_x - s * 0.5f, center_y - s * 0.5f, s, s};
}

/// 敌机护盾泡：以敌机矩形中心为中心，小怪 32、精英 48。
inline Rect EnemyShieldRect(const Layout& layout, int enemy_x, int enemy_y, int enemy_w,
                            int enemy_h, int size)
{
	const Rect body = EnemyRect(layout, enemy_x, enemy_y, enemy_w, enemy_h);
	return CenteredSquare(body.x + body.w * 0.5f, body.y + body.h * 0.5f, size);
}

/// 玩家护盾泡：以玩家精灵矩形中心为中心。
inline Rect PlayerShieldRect(const Layout& layout, int player_x, int player_y)
{
	const Rect body = PlayerRect(layout, player_x, player_y);
	return CenteredSquare(body.x + body.w * 0.5f, body.y + body.h * 0.5f, kShieldBubbleBigSize);
}

} // namespace View
