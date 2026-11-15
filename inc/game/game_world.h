#pragma once

#include <chrono>
#include <cstdint>
#include <random>
#include <vector>

#include "game/enemy.h"
#include "game/game_config.h"
#include "game/game_types.h"
#include "game/input_state.h"
#include "game/world_snapshot.h"

namespace thunder
{

struct Bullet
{
	int x;
	int y;

	Bullet(int _x = 0, int _y = 0): x(_x), y(_y) {}
};

/// 敌人被击毁后掉落的道具；与玩家接触即拾取。
/// 血包与弹药包共用同一套下落 / 判定逻辑，只有 kind 决定拾取效果。
struct Pickup
{
	int x;
	int y;
	PickupKind kind;

	Pickup(int _x = 0, int _y = 0, PickupKind _kind = PickupKind::Health): x(_x), y(_y), kind(_kind)
	{
	}
};

/// 平台无关的游戏世界。
/// 拥有全部可变对局状态；通过 Tick(input, dt) 推进，通过 Snapshot() 只读暴露。
/// 不读系统时钟、不轮询键盘、不做任何终端/图形输出。
class GameWorld
{
public:
	/// @param rng_seed 0 表示用 random_device 播种；非 0 时用于复现（测试/回放）。
	explicit GameWorld(std::uint32_t rng_seed = 0);

	/// 开新一局（重构前为 ThunderFighter::ResetGame）。
	/// 必须在进入 Playing 前调用。
	void Reset(Difficulty difficulty);

	/// 推进一帧。dt 由调用方提供；暂停时调用方不应调用本函数。
	void Tick(const GameInput& input, std::chrono::steady_clock::duration dt);

	/// 渲染与测试用的只读快照。
	WorldSnapshot Snapshot() const;

	bool Over() const
	{
		return life_ <= 0;
	}
	int Score() const
	{
		return score_;
	}
	Difficulty difficulty() const
	{
		return difficulty_;
	}

private:
	// ========== timing ==========
	std::chrono::steady_clock::duration elapsed_{};
	int elapsed_seconds_ = 0;
	int last_score_time_ = 0;
	int last_level_up_time_ = 0;

	// ========== progression ==========
	Difficulty difficulty_ = Difficulty::Easy;
	int level_ = 1;
	int score_ = 0;

	// ========== randomness ==========
	std::mt19937 rng_;

	// ========== player ==========
	int player_x_ = 0;
	int player_y_ = 0;
	int life_ = 0;
	bool is_invincible_ = false;
	bool cheat_invincible_ = false;
	std::chrono::steady_clock::duration invincible_remaining_{};

	// ========== bullets ==========
	std::vector<Bullet> bullets_;
	int bullets_remaining_ = 0;

	// ========== enemies ==========
	std::vector<Enemy> enemies_;
	std::vector<Enemy> pending_enemies_;
	int enemy_move_counter_ = 0;
	int enemy_move_interval_ = GameConfig::kInitialEnemyMoveInterval;
	int remaining_rows_in_batch_ = 0;

	// ========== pickups ==========
	std::vector<Pickup> pickups_;
	int pickup_move_counter_ = 0;

	// ========== update ==========
	void UpdateLevel(const GameInput& input);
	void AccrueScore();
	void UpdateBullets();
	void CheckPlayerCollision();
	void UpdatePickups();
	void CheckPickupCollect();
	void MaybeDropPickup(int enemy_x, int enemy_y);

	void SpawnEnemyWave();
	void ClearActiveEnemies();
	void SpawnEnemiesFromPending();
	void MoveEnemies(std::vector<Enemy>& active_enemies, int level, int screen_width,
	                 int screen_height);

	// ========== cheats ==========
	void CheatKillAll();
	void CheatAddLife();
	void CheatToggleInvincible();
	void CheatAddScore();
	void CheatAddAmmo();
	void CheatGodMode();
	void CheatSelfDestruct();

	// 测试专用访问器（定义位于 tests/world_test_access.h，不参与生产构建）
	friend struct WorldTestAccess;
};

} // namespace thunder
