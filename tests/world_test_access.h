#pragma once

#include <chrono>
#include <vector>

#include "game/game_world.h"

namespace thunder
{

/// 仅供测试使用的内部访问器。
/// 生产代码不包含本文件；GameWorld 中以 friend 声明保留入口。
struct WorldTestAccess
{
	static void ClearEnemies(GameWorld& world) { world.enemies_.clear(); }
	static void ClearPending(GameWorld& world)
	{
		world.pending_enemies_.clear();
	}
	static void ClearBullets(GameWorld& world) { world.bullets_.clear(); }

	static void AddEnemy(GameWorld& world, int x, int y, int speed = 1,
	                     int width = 3, int height = 1)
	{
		world.enemies_.emplace_back(x, y, speed, width, height);
	}

	static void AddBullet(GameWorld& world, int x, int y)
	{
		world.bullets_.emplace_back(x, y);
	}

	static void SetPlayer(GameWorld& world, int x, int y)
	{
		world.player_x_ = x;
		world.player_y_ = y;
	}

	static int EnemyCount(const GameWorld& world)
	{
		return static_cast<int>(world.enemies_.size());
	}

	static int PendingCount(const GameWorld& world)
	{
		return static_cast<int>(world.pending_enemies_.size());
	}

	static int BulletCount(const GameWorld& world)
	{
		return static_cast<int>(world.bullets_.size());
	}

	static int EnemyMoveInterval(const GameWorld& world)
	{
		return world.enemy_move_interval_;
	}

	static int LastLevelUpTime(const GameWorld& world)
	{
		return world.last_level_up_time_;
	}

	static int LastScoreTime(const GameWorld& world)
	{
		return world.last_score_time_;
	}

	static bool IsInvincible(const GameWorld& world)
	{
		return world.is_invincible_;
	}

	static bool CheatInvincible(const GameWorld& world)
	{
		return world.cheat_invincible_;
	}

	static std::chrono::steady_clock::duration InvincibleRemaining(
	    const GameWorld& world)
	{
		return world.invincible_remaining_;
	}
};

} // namespace thunder
