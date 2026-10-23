// 终端字符网格：把 GameWorld 的快照转换为 98x40 的字符缓冲区。
// 原实现内联在 ThunderFighter::DrawFrame 中（def.cpp），此处仅做搬移。

#include "cell_grid.h"

#include "game/game_config.h"

namespace CellGrid
{

std::vector<std::string> Build(const thunder::WorldSnapshot& snapshot,
                               bool paused, int width, int height)
{
	// 创建字符缓冲区
	std::vector<std::string> screen_buffer(height,
	                                       std::string(width, ' '));

	// 绘制敌人
	for(const auto& e: snapshot.enemies)
	{
		int draw_y = static_cast<int>(e.y);

		if(draw_y < 0 || draw_y >= height)
			continue;

		for(int i = 0;
		    i < GameConfig::kEnemyWidth && e.x + i < width;
		    i++)
		{
			if(e.x + i >= 0)
				screen_buffer[draw_y][e.x + i] = "$o$"[i];
		}
	}

	// 绘制子弹
	for(const auto& b: snapshot.bullets)
	{
		if(b.y >= 0 && b.y < height && b.x >= 0 && b.x < width)
		{
			screen_buffer[b.y][b.x] = GameConfig::kBulletChar;
		}
	}

	// 绘制玩家
	std::string player_str = "*0*";
	for(int i = 0; i < GameConfig::kPlayerWidth && snapshot.player_x + i < width;
	    ++i)
		screen_buffer[snapshot.player_y][snapshot.player_x + i] = player_str[i];

	// 暂停状态时显示暂停文字
	if(paused)
	{
		std::string pause_txt = "=== P A U S E D ===";
		int py = height / 2;
		int px = (width - (int)pause_txt.size()) / 2;
		for(int i = 0; i < (int)pause_txt.size(); ++i)
		{
			if(px + i >= 0 && px + i < width)
				screen_buffer[py][px + i] = pause_txt[i];
		}
	}

	return screen_buffer;
}

} // namespace CellGrid
