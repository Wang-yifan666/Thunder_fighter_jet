#pragma once

/// 界面上出现的中文文案（UTF-8）。
///
/// 内嵌的中文字体是**按这里用到的汉字子集化**的：改完文案必须重跑
///     python tools/subset_cjk_font.py
/// 否则新加的字会渲染成空白方块。
///
/// 只放"要显示的中文"，逻辑判断一律用枚举，不要拿文案当状态。

namespace GameText
{

// ========== 教程 ==========
inline constexpr const char* kTutorialMove = "试着用 W A S D 移动奶蛋！";
inline constexpr const char* kTutorialMoveHint = "W 上　A 左　S 下　D 右";

inline constexpr const char* kTutorialFire = "按住 Space 开火！注意，弹药是有限的。";
inline constexpr const char* kTutorialFireHint = "教程里打空了也没关系，补给管够";

inline constexpr const char* kTutorialKill = "击落那只小恶魔，击败敌机会补充弹药！";
inline constexpr const char* kTutorialKillHint = "它走得很慢，而且不会还手";

inline constexpr const char* kTutorialPickup = "把这四种补给都捡起来：";
inline constexpr const char* kTutorialPickupHint = "红 回血　　黄 弹药　　蓝 护盾　　紫 三路火力";

inline constexpr const char* kTutorialDone = "奶蛋已经准备好迎接真正的战斗了！";
inline constexpr const char* kTutorialUnlock = "解锁了新装扮：鲨鱼罩";
inline constexpr const char* kTutorialSkip = "Q 跳过教程";
inline constexpr const char* kTutorialStart = "1 开始游戏";
inline constexpr const char* kTutorialBack = "2 返回主菜单";

// ========== 装扮 ==========
inline constexpr const char* kSkinsTitle = "选择装扮";
inline constexpr const char* kSkinsLocked = "通关教程后解锁";
inline constexpr const char* kSkinsHint = "A / D 切换　1 确认　Q 返回";
inline constexpr const char* kSkinDefault = "奶蛋";
inline constexpr const char* kSkinShark = "鲨鱼罩";

} // namespace GameText
