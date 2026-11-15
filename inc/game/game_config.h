#pragma once

namespace GameConfig
{

// ========== 屏幕 / 窗口 ==========
constexpr int kScreenWidth = 102; // 棋盘格数
constexpr int kScreenHeight = 48;

// ========== 帧 / 菜单延迟 (ms) ==========
// 游戏逻辑固定每步 16ms（约 62.5 步/秒），渲染帧率与它解耦。
constexpr int kFrameDelayMs = 16;
constexpr int kKeyDebounceMs = 200;     // 按键防抖
constexpr int kAntiBounceDelayMs = 120; // 防抖延迟

// ========== 游戏核心数值 ==========
constexpr int kInitialEnemyMoveInterval = 20;
constexpr int kScorePerSecond = 10;
constexpr int kScorePerKill = 100;
constexpr int kBulletsPerKill = 5;
constexpr int kBulletsPerLevelUp = 10;
constexpr int kEndGameBulletBonus = 10;

// ========== 难度相关 ==========
constexpr int kEasyLife = 8;
constexpr int kEasyBullets = 300;
constexpr int kNormalLife = 5;
constexpr int kNormalBullets = 200;
constexpr int kHardLife = 3;
constexpr int kHardBullets = 150;

// ========== 敌人 ==========
constexpr int kEnemyWidth = 3;
constexpr int kEnemyHeight = 2; // 原 1 行；精灵需要方形空间，命中框同步加高
constexpr int kMinEnemySpeed = 1;
constexpr int kMaxEnemySpeed = 3;
constexpr int kMinEnemyMoveInterval = 5;
constexpr int kMaxEnemyMoveInterval = 20;
constexpr int kLevelMoveIntervalDivisor = 9;
constexpr int kLevelUpTime = 10; // 升级间隔(秒)
constexpr int kStartToMoveLeftRight = 4;

// ========== 刷怪批次 ==========
constexpr int kEasyMaxBatchCount = 5;
constexpr int kEasyMaxPerBatch = 2;
constexpr int kNormalMaxBatchCount = 15;
constexpr int kNormalMaxPerBatch = 3;
constexpr int kHardMaxBatchCount = 20;
constexpr int kHardMaxPerBatch = 5;

// ========== 玩家 ==========
constexpr int kPlayerWidth = 3;
constexpr int kPlayerHeight = 1;
constexpr int kInvincibleDurationSec = 1;

// 拾取判定用的"视觉身体"高度（格）。
// 精灵是 32x32 像素、底边对齐命中框底边，肉眼看到的身体有 4 行高；
// 受伤害仍用 kPlayerHeight(1 行) 的命中框，两者必须分开：
// 拿 1 行的伤害判定去做拾取，会出现"从道具上碾过去却没吃到"。
constexpr int kPlayerGrabHeight = 4;

// ========== 掉落物（血包 / 弹药包） ==========
constexpr int kPickupSize = 3;          // 3x3 格，精灵与判定等大
constexpr int kPickupSpeed = 1;         // 每次移动的格子数
constexpr int kPickupMoveInterval = 16; // 每多少帧移动一次（约 12 秒穿过全屏）
constexpr int kHealthPickupLife = 1;    // 血包回复的生命值
constexpr int kAmmoPickupBullets = 50;  // 弹药包补充的子弹数

// 掉落率随难度变化（百分比）。
// 难度越低补给越慷慨：Easy 是新手保护，Hard 靠自己本事——高手不需要救济。
constexpr int kEasyHealthDropPercent = 10;
constexpr int kEasyAmmoDropPercent = 12;
constexpr int kNormalHealthDropPercent = 8;
constexpr int kNormalAmmoDropPercent = 8;
constexpr int kHardHealthDropPercent = 6;
constexpr int kHardAmmoDropPercent = 6;

// ========== 作弊 ==========
constexpr int kCheatAddScore = 1000;
constexpr int kCheatAddBullets = 100;
constexpr int kCheatGodlikeLife = 9999999;
constexpr int kCheatGodlikeBullets = 9999999;
constexpr int kCheatNoticeMs = 1500; // 切换作弊模式后提示显示时长

// ========== 显示 ==========
constexpr int kMaxHighScores = 5;

} // namespace GameConfig