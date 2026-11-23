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
	Skins,
	Tutorial,
	TutorialDone,
	Playing,
	GameOver
};

struct SessionSnapshot
{
	Screen screen = Screen::Menu;
	bool paused = false;
	bool exit_requested = false;
	Difficulty difficulty = Difficulty::Easy;

	/// 作弊模式是否已开启（主菜单按 F 切换）。关闭时数字键作弊不生效。
	bool cheats_enabled = false;

	/// 刚切换过作弊模式：前端应短暂显示一次开/关提示，之后不再显示。
	bool cheat_notice = false;

	/// 教程当前步骤（仅 Screen::Tutorial 有意义）
	TutorialStep tutorial_step = TutorialStep::Move;
	/// 当前这一步的目标已经达成（前端可以显示"做得好"）
	bool tutorial_step_done = false;
	/// 本次是否刚刚解锁了装扮（第二次通关不再提示）
	bool tutorial_newly_unlocked = false;
	/// 本次教程是否全部走完（按 Q 跳过则为 false）
	bool tutorial_solved = false;
	/// 存档里是否已有"通关教程"的记录（决定装扮是否解锁）
	bool tutorial_done = false;

	/// 装扮：当前生效的 id，以及装扮界面里高亮的那一个
	int skin_id = 0;
	int skin_cursor = 0;
	/// 鲨鱼罩是否已解锁（通关教程）
	bool shark_unlocked = false;

	/// 仅 Playing / GameOver / Tutorial 有意义
	WorldSnapshot world;

	// 排行榜 / 结算
	std::vector<int> leaderboard;
	int rank = 0;
	bool new_record = false;
	bool entered_top5 = false;
	int final_score = 0; ///< 结算成绩（含剩余弹药加成），未结算时为 0
};

} // namespace thunder
