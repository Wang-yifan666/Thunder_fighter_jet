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
inline constexpr const char* kTutorialKillHint = "击落敌机会补充弹药，所以别怕打空";

inline constexpr const char* kTutorialPickupHealth = "红 血包：生命 +1";
inline constexpr const char* kTutorialPickupHealthHint = "撞到敌机掉 1 条命，捡血包补回来";

inline constexpr const char* kTutorialPickupAmmo = "黄 弹药包：子弹 +50 发";
inline constexpr const char* kTutorialPickupAmmoHint = "弹药打光就开不了火，靠它续命";

inline constexpr const char* kTutorialPickupShield = "蓝 护盾包：挡一次撞击";
inline constexpr const char* kTutorialPickupShieldHint = "破盾时会放出一圈冲击波，扫掉身边的敌人";

inline constexpr const char* kTutorialPickupFirepower = "紫 火力包：15 秒三路开火";
inline constexpr const char* kTutorialPickupFirepowerHint = "一次打出三发，但只消耗 1 发弹药";

inline constexpr const char* kTutorialMenu = "3  新手教程";
inline constexpr const char* kTutorialWellDone = "做得好！";
inline constexpr const char* kTutorialDone = "奶蛋已经准备好迎接真正的战斗了！";
inline constexpr const char* kTutorialUnlock = "解锁了新装扮：鲨鱼罩";
inline constexpr const char* kTutorialSkip = "Q 跳过教程";
inline constexpr const char* kTutorialStart = "1 开始游戏";
inline constexpr const char* kTutorialBack = "2 返回主菜单";

// ========== 玩法说明 ==========
inline constexpr const char* kHelpMenu = "4  玩法说明";
inline constexpr const char* kHelpTitle = "玩法说明";

inline constexpr const char* kHelpLine1 = "移动 W A S D　　开火 Space（每 2 帧一发）";
inline constexpr const char* kHelpLine2 = "弹药：击落敌机 +5 发，每升 1 级再 +10 发，打光就打不出子弹";
inline constexpr const char* kHelpLine3 = "生命：撞到敌机掉 1 条命，之后 1 秒无敌";
inline constexpr const char* kHelpLine4 = "升级：每 10 秒升 1 级，敌人更快更密（第 1 级只等 5 秒）";
inline constexpr const char* kHelpLine5 = "精英：9 级起出现，4x3 格、2 血、2.5 倍分数、+8 发弹药";
inline constexpr const char* kHelpLine6 = "护盾怪：6 级起小怪有概率带盾，先破盾再补一发";
inline constexpr const char* kHelpLine7 = "四种补给：红 血包　黄 弹药包　蓝 护盾包　紫 火力包";
inline constexpr const char* kHelpLine8 = "护盾包挡一次撞击，破盾时放冲击波；火力包 15 秒三路开火";
inline constexpr const char* kHelpBack = "B / Q 返回";

// ========== 装扮 ==========
inline constexpr const char* kSkinsTitle = "选择装扮";
inline constexpr const char* kSkinsLocked = "通关教程后解锁";
inline constexpr const char* kSkinsHint = "A / D 切换　1 确认　Q 返回";
inline constexpr const char* kSkinDefault = "奶蛋";
inline constexpr const char* kSkinShark = "鲨鱼罩";

} // namespace GameText
