#pragma once

/// 难度档位。界面流程状态由 SessionSnapshot 的 Screen 枚举描述。
enum class Difficulty
{
	Easy,
	Normal,
	Hard
};

/// 掉落物的种类。外观与拾取效果都由它决定，逻辑层只认种类。
enum class PickupKind
{
	Health,   ///< 回血
	Ammo,     ///< 补弹药
	Shield,   ///< 玩家获得一层护盾（破盾时放冲击波）
	Firepower ///< 15 秒三路开火
};

/// 敌机种类。普通小怪 3x2 格、1 血；精英 4x3 格、2 血、可能自带护盾。
enum class EnemyKind
{
	Normal,
	Elite
};

/// 纯视觉特效的种类（逻辑层只记位置与寿命，怎么画由前端决定）。
enum class EffectKind
{
	Spark,    ///< 子弹 / 冲击波命中敌机的火花
	Shockwave ///< 玩家破盾冲击波
};
