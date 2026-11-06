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
	Health, ///< 回血
	Ammo    ///< 补弹药
};
