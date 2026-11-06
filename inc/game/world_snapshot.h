#pragma once

#include <vector>

#include "game/game_types.h"

namespace thunder
{

/// 单个可绘制实体（敌机 / 子弹）的只读视图。
struct EntityView
{
	int x = 0;
	int y = 0;
	int width = 0;
	int height = 0;
};

/// 掉落物的只读视图：位置尺寸之外还带种类，前端据此选外观。
struct PickupView
{
	int x = 0;
	int y = 0;
	int width = 0;
	int height = 0;
	PickupKind kind = PickupKind::Health;
};

/// 游戏世界的只读快照。
/// 只描述"世界"本身：菜单、暂停、排行榜、退出请求属于会话层，不在这里。
struct WorldSnapshot
{
	Difficulty difficulty = Difficulty::Easy;

	// 玩家
	int player_x = 0;
	int player_y = 0;
	int life = 0;
	bool invincible = false; // is_invincible_ || cheat_invincible_

	// 场景
	std::vector<EntityView> enemies;
	std::vector<EntityView> bullets;
	std::vector<PickupView> pickups;

	// HUD 数值
	int score = 0;
	int level = 1;
	int bullets_remaining = 0;
	int elapsed_seconds = 0;
	int pending_enemies = 0;

	bool over = false;
};

} // namespace thunder
