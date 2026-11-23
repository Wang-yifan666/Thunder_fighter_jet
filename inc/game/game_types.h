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

/// 对局模式：正常对局 / 新手教程。
/// 教程模式只关掉"自动刷波、升级、掉落、掉血"，其余（移动、开火、命中、拾取）完全复用。
enum class GameMode
{
	Normal,
	Tutorial
};

/// 可选装扮数量。0 号是默认奶蛋，1 号是鲨鱼罩（通关教程解锁）。
constexpr int kSkinCount = 2;
/// 鲨鱼罩的 id（解锁条件写在会话层）
constexpr int kSkinSharkId = 1;

/// 教程的步骤。逻辑层只认枚举，文案在 game_text.h，画法由前端决定。
/// 四种补给是一个一个来的：每步只摆一个、只讲一种，捡到才进下一种。
enum class TutorialStep
{
	Move,             ///< 让玩家动一下
	Fire,             ///< 让玩家开一枪
	Kill,             ///< 击落两只慢速小怪
	PickupHealth,     ///< 捡血包，讲回血
	PickupAmmo,       ///< 捡弹药包，讲弹药
	PickupShield,     ///< 捡护盾包，讲护盾与冲击波
	PickupFirepower,  ///< 捡火力包，讲三路开火
	LevelUp,          ///< 真升一级（HUD 的 LEVEL 会变），讲升级机制
	Mechanics,        ///< 只读一页机制说明（精英/护盾怪），到时间自动过
	Done              ///< 全部完成
};