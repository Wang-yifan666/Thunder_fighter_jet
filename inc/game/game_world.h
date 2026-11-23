#pragma once

#include <chrono>
#include <cstdint>
#include <deque>
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
/// 四种道具共用同一套下落 / 判定逻辑，只有 kind 决定拾取效果。
struct Pickup
{
	int x;
	int y;
	PickupKind kind;

	Pickup(int _x = 0, int _y = 0, PickupKind _kind = PickupKind::Health): x(_x), y(_y), kind(_kind)
	{
	}
};

/// 纯视觉特效：逻辑层只记位置与寿命，怎么画由前端决定（不影响任何判定）。
struct Effect
{
	int x; ///< 格子坐标
	int y;
	EffectKind kind;
	int age; ///< 已存活帧数
	int life; ///< 总帧数
	float radius; ///< 冲击波半径（格），其它特效为 0

	Effect(int _x, int _y, EffectKind _kind, int _life, float _radius = 0.0f):
	x(_x), y(_y), kind(_kind), age(0), life(_life), radius(_radius)
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
	/// 教程模式不刷波、不升级、不结算，玩家也不会掉血。
	void Reset(Difficulty difficulty, GameMode mode = GameMode::Normal);

	GameMode mode() const
	{
		return mode_;
	}

	// ========== 教程脚本专用 ==========
	// 教程的编排放在会话层（GameSession），这里只提供"摆一只/摆一堆"的最小入口，
	// 不经过刷怪与掉落系统，数值完全由调用方指定。

	/// 摆一只直线下落的小怪（教程目标是"不会还手、走得很慢"那种）。
	void TutorialSpawnEnemy(int cell_x, int cell_y, int speed);
	/// 摆一个补给。
	void TutorialSpawnPickup(int cell_x, int cell_y, PickupKind kind);
	/// 摆好玩家的位置。
	void TutorialSetPlayer(int cell_x, int cell_y);
	/// 让教程真的升一级（HUD 的 LEVEL 会 +1，敌人移动间隔按等级重算）。
	void TutorialAdvanceLevel();
	/// 保证弹药至少还有 at_least 发：教程里打光了就会卡在"要求开火"的步骤上。
	void TutorialEnsureAmmo(int at_least);

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
	int DefeatedCount() const
	{
		return defeated_count_;
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
	GameMode mode_ = GameMode::Normal;
	Difficulty difficulty_ = Difficulty::Easy;
	int level_ = 1;
	int score_ = 0;
	/// 本局累计击毁的敌机数（教程用它判断"真的打掉了"，而不是敌人自己掉出屏幕）
	int defeated_count_ = 0;

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
	// 排队中的敌机：只从头部取、尾部放回，用 deque 而不是 vector，
	// 否则每次 pop 头部都要搬整个数组（后期这里能积压几百上千只）。
	std::deque<Enemy> pending_enemies_;
	int enemy_move_counter_ = 0;
	int enemy_move_interval_ = GameConfig::kInitialEnemyMoveInterval;
	int remaining_rows_in_batch_ = 0;

	// ========== pickups ==========
	std::vector<Pickup> pickups_;
	int pickup_move_counter_ = 0;

	// ========== 射击 / 护盾 / 特效 ==========
	int fire_cooldown_frames_ = 0;   // 距离下一次开火还要等几帧
	int firepower_frames_left_ = 0;  // 火力强化剩余帧数（0 = 单路）
	bool player_shield_ = false;     // 玩家是否还有一层护盾
	std::vector<Effect> effects_;    // 纯视觉特效（火花 / 冲击波）

	// ========== update ==========
	void UpdateLevel(const GameInput& input);
	/// 按当前等级重算敌机移动间隔（正常升级与教程升级共用）
	void RefreshEnemyMoveInterval();
	void AccrueScore();
	void UpdateBullets();
	void UpdateEffects();
	void CheckPlayerCollision();
	void UpdatePickups();
	void CheckPickupCollect();
	void MaybeDropPickup(int enemy_x, int enemy_y);
	void FirePlayerBullets();
	void AddEffect(EffectKind kind, int cell_x, int cell_y, int life);

	/// 一次命中的结果：无敌帧穿过 / 破盾 / 掉血 / 击毁。
	enum class HitResult
	{
		Blocked,
		ShieldBroken,
		Damaged,
		Destroyed
	};

	/// 对一只敌机结算一次伤害（先破盾、遵守无敌帧），击毁时负责加分、给弹药、掉落。
	/// hit_x / hit_y 是命中点的格子坐标，只用来放火花特效。
	HitResult ApplyHit(Enemy& enemy, int hit_x, int hit_y);

	/// 玩家破盾冲击波：圆形范围内每只敌机吃一次伤害。
	void ApplyShockwave();

	/// 当前等级下精英的出现概率（百分比）。
	int EliteSpawnPercent() const;

	/// 拥堵泄压：场上 + 排队的小怪超过阈值时，每次刷怪额外多放几只。
	static int CrowdBatchBonus(int backlog);

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
