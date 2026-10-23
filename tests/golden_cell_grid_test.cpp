// 终端字符网格的 golden 测试：用人工构造的 WorldSnapshot 锁定字形与布局，
// 不依赖随机种子，也不依赖 GameWorld 的具体数值。

#include <gtest/gtest.h>

#include <string>

#include "cell_grid.h"
#include "game/game_config.h"
#include "game/game_types.h"
#include "game/world_snapshot.h"
#include "render_console.h"

using thunder::EntityView;
using thunder::WorldSnapshot;

namespace
{

WorldSnapshot MakeSnapshot()
{
	const int player_x = GameConfig::kScreenWidth / 2 - 1;
	const int player_y = GameConfig::kScreenHeight - 1;

	WorldSnapshot snapshot;
	snapshot.difficulty = Difficulty::Hard;
	snapshot.player_x = player_x;
	snapshot.player_y = player_y;
	snapshot.life = 3;
	snapshot.invincible = false;

	snapshot.enemies = {
	    EntityView{0, 0, 3, 1},
	    EntityView{10, 5, 3, 1},
	    EntityView{GameConfig::kScreenWidth - 2, 1, 3, 1}, // 右侧被裁剪
	    EntityView{player_x + 2, player_y, 3, 1}, // 与玩家同一行，验证绘制顺序
	};

	snapshot.bullets = {
	    EntityView{player_x + 1, player_y - 1, 1, 1},
	    EntityView{-1, 20, 1, 1}, // 越界，忽略
	    EntityView{GameConfig::kScreenWidth, 20, 1, 1}, // 越界，忽略
	};

	snapshot.score = 123;
	snapshot.level = 2;
	snapshot.bullets_remaining = 7;
	snapshot.elapsed_seconds = 42;
	snapshot.pending_enemies = 1;

	return snapshot;
}

} // namespace

TEST(CellGrid, DrawsEntitiesAndClipsRightEdge)
{
	const WorldSnapshot snapshot = MakeSnapshot();
	const auto grid = CellGrid::Build(snapshot, false,
	                                  GameConfig::kScreenWidth,
	                                  GameConfig::kScreenHeight);

	ASSERT_EQ(grid.size(), static_cast<size_t>(GameConfig::kScreenHeight));
	for(const auto& row: grid)
		EXPECT_EQ(row.size(), static_cast<size_t>(GameConfig::kScreenWidth));

	// 敌人字形与位置
	EXPECT_EQ(grid[0].substr(0, 3), "$o$");
	EXPECT_EQ(grid[5].substr(10, 3), "$o$");

	// 右侧只剩两格（宽度 - 2 开始）
	EXPECT_EQ(grid[1].substr(GameConfig::kScreenWidth - 2, 2), "$o");

	// 子弹
	EXPECT_EQ(grid[snapshot.player_y - 1][snapshot.player_x + 1],
	          GameConfig::kBulletChar);

	// 玩家先画敌机、再覆盖玩家；同一行:
	// 玩家 "*0*" 覆盖前三格，后面保留敌机的 "o$"
	EXPECT_EQ(grid[snapshot.player_y].substr(snapshot.player_x, 5),
	          "*0*o$");

	// 越界子弹不产生任何字符
	EXPECT_EQ(grid[20], std::string(GameConfig::kScreenWidth, ' '));
}

TEST(CellGrid, NoPauseOverlayWhenRunning)
{
	const auto grid = CellGrid::Build(MakeSnapshot(), false,
	                                  GameConfig::kScreenWidth,
	                                  GameConfig::kScreenHeight);

	const int py = GameConfig::kScreenHeight / 2;
	EXPECT_EQ(grid[py], std::string(GameConfig::kScreenWidth, ' '));
}

TEST(CellGrid, PauseOverlayIsCentered)
{
	const auto grid = CellGrid::Build(MakeSnapshot(), true,
	                                  GameConfig::kScreenWidth,
	                                  GameConfig::kScreenHeight);

	const std::string pause_text = "=== P A U S E D ===";
	const int py = GameConfig::kScreenHeight / 2;
	const int px =
	    (GameConfig::kScreenWidth - static_cast<int>(pause_text.size())) / 2;

	// 居中：左右留白相差不超过 1 格（宽度为奇数时允许偏 1）
	const int right = GameConfig::kScreenWidth - px
	    - static_cast<int>(pause_text.size());
	const int skew = px > right ? px - right : right - px;
	EXPECT_LE(skew, 1);
	EXPECT_EQ(grid[py].substr(px, pause_text.size()), pause_text);
	EXPECT_EQ(grid[py].substr(0, px), std::string(px, ' '));
	EXPECT_EQ(grid[py].substr(px + pause_text.size(),
	                           GameConfig::kScreenWidth - px
	                               - pause_text.size()),
	          std::string(GameConfig::kScreenWidth - px
	                          - pause_text.size(),
	                      ' '));
}

TEST(RenderConsole, LifeStringFormatsHealth)
{
	EXPECT_EQ(RenderConsole::LifeString(0, 10), "剩余生命: ");
	EXPECT_EQ(RenderConsole::LifeString(3, 10), "剩余生命: <> <> <> ");

	std::string expected = "剩余生命: ";
	for(int i = 0; i < 9; ++i)
		expected += "<> ";
	expected += " ++ ";
	EXPECT_EQ(RenderConsole::LifeString(12, 10), expected);
}
