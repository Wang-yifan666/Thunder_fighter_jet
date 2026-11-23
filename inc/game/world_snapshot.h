#pragma once

#include <vector>

#include "game/game_types.h"

namespace thunder
{

/// 单个可绘制实体（子弹）的只读视图。
struct EntityView
{
	int x = 0;
	int y = 0;
	int width = 0;
	int height = 0;
};

/// 敌机的只读视图：位置尺寸之外还带种类与战斗状态，前端据此选贴图、画护盾。
struct EnemyView
{
	int x = 0;
	int y = 0;
	int width = 0;
	int height = 0;
	EnemyKind kind = EnemyKind::Normal;
	bool shielded = false;     ///< 还带着护盾（先破盾才能扣血）
	bool invulnerable = false; ///< 破盾后的无敌帧，子弹会穿过去
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

/// 纯视觉特效的只读视图：逻辑层只给位置、种类和进度，具体画法由前端决定。
struct EffectView
{
	int x = 0; ///< 特效中心的格子坐标
	int y = 0;
	EffectKind kind = EffectKind::Spark;
	int age = 0; ///< 已经存活多少帧
	int life = 1; ///< 总共存活多少帧
	float radius = 0.0f; ///< 冲击波半径（格）；其它特效为 0
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
	bool invincible = false;        // is_invincible_ || cheat_invincible_
	bool shielded = false;          // 还有一层护盾
	int firepower_seconds_left = 0; // 火力强化剩余秒数（0 = 单路开火）

	// 场景
	std::vector<EnemyView> enemies;
	std::vector<EntityView> bullets;
	std::vector<PickupView> pickups;
	std::vector<EffectView> effects;

	// HUD 数值
	int score = 0;
	int level = 1;
	int bullets_remaining = 0;
	int elapsed_seconds = 0;
	int pending_enemies = 0;
	int defeated = 0; ///< 本局累计击毁数量（从底部漏掉的不算）

	bool over = false;
};

} // namespace thunder
