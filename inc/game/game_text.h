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
inline constexpr const char* kTutorialFireHint = "击落一架敌机补 5 发，每升 1 级再补 10 发";

inline constexpr const char* kTutorialKill = "击落这两只小恶魔，它们不会还手。";
inline constexpr const char* kTutorialKillHint = "撞到敌机会掉 1 条命，之后 1 秒无敌";

inline constexpr const char* kTutorialPickupHealth = "红 血包：生命 +1";
inline constexpr const char* kTutorialPickupHealthHint = "撞到敌机掉 1 条命，捡血包补回来";

inline constexpr const char* kTutorialPickupAmmo = "黄 弹药包：子弹 +50 发";
inline constexpr const char* kTutorialPickupAmmoHint = "看顶部的 AMMO：捡到之后会一次涨 50";

inline constexpr const char* kTutorialPickupShield = "蓝 护盾包：挡一次撞击";
inline constexpr const char* kTutorialPickupShieldHint = "撞到敌机时先破盾、不掉血，破盾瞬间放出一圈冲击波扫掉身边敌人";

inline constexpr const char* kTutorialPickupFirepower = "紫 火力包：15 秒三路开火";
inline constexpr const char* kTutorialPickupFirepowerHint = "一次打出三发，但只消耗 1 发弹药";

inline constexpr const char* kTutorialMenu = "3  新手教程";
inline constexpr const char* kTutorialLevelUp = "升级！看顶部 LEVEL 变成了 2，敌人更快了";
inline constexpr const char* kTutorialLevelUpHint = "每 10 秒升 1 级，等级越高敌人越快越密";
inline constexpr const char* kTutorialMechanics = "再往后：9 级起出现精英，6 级起小怪可能带盾";
inline constexpr const char* kTutorialMechanicsHint = "精英 2 血、2.5 倍分、+8 发弹药；带盾的小怪要先破盾再补一发";
inline constexpr const char* kTutorialWellDone = "做得好！";
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
