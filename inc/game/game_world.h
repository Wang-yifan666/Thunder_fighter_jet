#pragma once

#include <chrono>
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

/// 平台无关的游戏世界。
/// 拥有全部可变对局状态；通过 Tick(input, dt) 推进，通过 Snapshot() 只读暴露。
/// 不读系统时钟、不轮询键盘、不做任何终端/图形输出。
class GameWorld
{
public:
	GameWorld() = default;

	/// 开新一局（重构前为 ThunderFighter::ResetGame）。
	/// 必须在进入 Playing 前调用。
	void Reset(Difficulty difficulty);

	/// 推进一帧。dt 由调用方提供；暂停时调用方不应调用本函数。
	void Tick(const GameInput& input, std::chrono::steady_clock::duration dt);

	/// 渲染与测试用的只读快照。
	WorldSnapshot Snapshot() const;

	bool Over() const { return life_number <= 0; }
	int Life() const { return life_number; }
	int Score() const { return score_; }
	Difficulty difficulty() const { return difficulty_; }

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

	// ========== player ==========
	int player_x_ = 0;
	int player_y_ = 0;
	int life_number = 0;
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

	// ========== update ==========
	void Level(const GameInput& input);
	void Score();
	void UpdateBullets();
	void CheckPlayerCollision();

	void Make_enermy();
	void Clear_Current_Enemies();
	void SpawnEnemiesFromPending();
	bool IsOverlapping(const Enemy& a, const Enemy& b) const;
	bool WillOverlap(const Enemy& cur, const std::vector<Enemy>& enemies,
	                 int new_x, int new_y) const;
	void MoveEnemies(std::vector<Enemy>& active_enemies, int level,
	                 int screen_width, int screen_height);

	// ========== cheats ==========
	void cheats_kills();
	void cheats_life();
	void cheats_invincible();
	void cheats_addscore();
	void cheats_addbullters();
	void cheats_godlike();
	void kill_self();
};

} // namespace thunder
