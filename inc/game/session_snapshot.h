#pragma once

#include <vector>

#include "game/game_types.h"
#include "game/world_snapshot.h"

namespace thunder
{

/// 会话层的界面状态。菜单/暂停/排行榜只属于这里，不进 WorldSnapshot。
enum class Screen
{
	Menu,
	DifficultySelect,
	Leaderboard,
	Playing,
	GameOver
};

struct SessionSnapshot
{
	Screen screen = Screen::Menu;
	bool paused = false;
	bool exit_requested = false;
	Difficulty difficulty = Difficulty::Easy;

	/// 仅 Playing / GameOver 有意义
	WorldSnapshot world;

	// 排行榜 / 结算
	std::vector<int> leaderboard;
	int rank = 0;
	bool new_record = false;
	bool entered_top5 = false;
	int final_score = 0; ///< 结算成绩（含剩余弹药加成），未结算时为 0
};

} // namespace thunder
