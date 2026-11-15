// 平台无关布局换算的回归测试。
//
// 这些断言的价值：把 sdl_render 里原先手写的像素公式固化下来。
// 重构（把布局抽到 View）不允许改变任何一像素的位置，
// 所以这里直接用"重构前的原始表达式"作为期望值。

#include <gtest/gtest.h>

#include "game/game_config.h"
#include "view/scene_layout.h"

namespace
{

const View::Layout kDefault;

} // namespace

TEST(ViewLayout, DefaultsMatchDesktopWindow)
{
	EXPECT_EQ(kDefault.window_width, 832);
	EXPECT_EQ(kDefault.window_height, 496);
	EXPECT_EQ(kDefault.cell, 8);
	EXPECT_EQ(kDefault.grid_x, 8);
	EXPECT_EQ(kDefault.grid_y, 64);
	EXPECT_EQ(kDefault.GridPixelWidth(), 102 * 8);
	EXPECT_EQ(kDefault.GridPixelHeight(), 48 * 8);
}

TEST(ViewLayout, CellOriginMatchesLegacyExpression)
{
	for(int cx = 0; cx < GameConfig::kScreenWidth; ++cx)
	{
		const float legacy =
		    static_cast<float>(kDefault.grid_x) + cx * static_cast<float>(kDefault.cell);
		EXPECT_EQ(View::CellX(kDefault, cx), legacy) << "cx=" << cx;
	}

	for(int cy = 0; cy < GameConfig::kScreenHeight; ++cy)
	{
		const float legacy =
		    static_cast<float>(kDefault.grid_y) + cy * static_cast<float>(kDefault.cell);
		EXPECT_EQ(View::CellY(kDefault, cy), legacy) << "cy=" << cy;
	}
}

TEST(ViewLayout, PlayerRectMatchesLegacyFormula)
{
	const float cell = static_cast<float>(kDefault.cell);
	const float size = static_cast<float>(View::kPlayerSpriteSize);

	for(int py = 0; py < GameConfig::kScreenHeight; ++py)
	{
		for(int px = 0; px <= GameConfig::kScreenWidth - GameConfig::kPlayerWidth; ++px)
		{
			// 重构前：x = grid_x + px*cell
			//         left = x + kPlayerWidth*cell/2 - size/2
			//         y = grid_y + py*cell；dst.y = y + cell - size
			const float x = static_cast<float>(kDefault.grid_x) + px * cell;
			const float y = static_cast<float>(kDefault.grid_y) + py * cell;
			const float left = x + GameConfig::kPlayerWidth * cell / 2.0f - size / 2.0f;
			const float top = y + cell - size;

			const View::Rect rect = View::PlayerRect(kDefault, px, py);
			EXPECT_EQ(rect.x, left) << "px=" << px << " py=" << py;
			EXPECT_EQ(rect.y, top) << "px=" << px << " py=" << py;
			EXPECT_EQ(rect.w, size);
			EXPECT_EQ(rect.h, size);
		}
	}
}

TEST(ViewLayout, PlayerSpriteIsWiderThanHitboxAndBottomAligned)
{
	const int px = 40;
	const int py = 30;
	const View::Rect rect = View::PlayerRect(kDefault, px, py);
	const View::Rect hitbox =
	    View::CellRect(kDefault, px, py, GameConfig::kPlayerWidth, GameConfig::kPlayerHeight);

	// 底边对齐
	EXPECT_EQ(rect.y + rect.h, hitbox.y + hitbox.h);
	// 视觉比判定宽（左右各悬垂同样多）
	EXPECT_GT(rect.w, hitbox.w);
	EXPECT_EQ(hitbox.x + hitbox.w * 0.5f, rect.x + rect.w * 0.5f);
}

TEST(ViewLayout, EnemyRectEqualsHitbox)
{
	for(int y = 0; y < 10; ++y)
	{
		const View::Rect rect =
		    View::EnemyRect(kDefault, 7, y, GameConfig::kEnemyWidth, GameConfig::kEnemyHeight);
		const View::Rect cell_rect =
		    View::CellRect(kDefault, 7, y, GameConfig::kEnemyWidth, GameConfig::kEnemyHeight);
		EXPECT_EQ(rect.x, cell_rect.x);
		EXPECT_EQ(rect.y, cell_rect.y);
		EXPECT_EQ(rect.w, cell_rect.w);
		EXPECT_EQ(rect.h, cell_rect.h);
		EXPECT_EQ(rect.w, static_cast<float>(View::kEnemySpriteWidth));
		EXPECT_EQ(rect.h, static_cast<float>(View::kEnemySpriteHeight));
	}
}

TEST(ViewLayout, PickupRectAppliesBob)
{
	const View::Rect plain = View::PickupRect(kDefault, 10, 20, 3, 3);
	const View::Rect bobbed = View::PickupRect(kDefault, 10, 20, 3, 3, 1.2f);

	EXPECT_EQ(bobbed.x, plain.x);
	EXPECT_EQ(bobbed.y, plain.y + 1.2f);
	EXPECT_EQ(bobbed.w, plain.w);
	EXPECT_EQ(bobbed.h, plain.h);
	EXPECT_EQ(plain.w, static_cast<float>(View::kPickupSpriteSize));
}

TEST(ViewLayout, BulletIsCenteredOnCell)
{
	const View::Rect rect = View::BulletRect(kDefault, 5, 9, 4.0f, 8.0f);
	const float cx = static_cast<float>(kDefault.grid_x) + 5 * static_cast<float>(kDefault.cell)
	    + static_cast<float>(kDefault.cell) * 0.5f;
	const float cy = static_cast<float>(kDefault.grid_y) + 9 * static_cast<float>(kDefault.cell)
	    + static_cast<float>(kDefault.cell) * 0.5f;

	EXPECT_EQ(rect.x, cx - 2.0f);
	EXPECT_EQ(rect.y, cy - 4.0f);
	EXPECT_EQ(rect.w, 4.0f);
	EXPECT_EQ(rect.h, 8.0f);
}
