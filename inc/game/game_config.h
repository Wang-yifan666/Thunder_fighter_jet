#pragma once

namespace GameConfig
{

// ========== 屏幕 / 窗口 ==========
constexpr int kScreenWidth = 98;
constexpr int kScreenHeight = 48; // 原 40 行，上下各扩充 4 行
constexpr int kWindowWidth = 100;
constexpr int kWindowHeight = 58; // 棋盘 48 行 + HUD/边框余量

// ========== 帧 / 菜单延迟 (ms) ==========
constexpr int kFrameDelayMs = 16;          // 主循环帧延迟 ~60fps
constexpr int kKeyDebounceMs = 200;        // 按键防抖
constexpr int kAntiBounceDelayMs = 120;    // 防抖延迟

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
constexpr int kMinEnemySpeed = 1;
constexpr int kMaxEnemySpeed = 3;
constexpr int kMinEnemyMoveInterval = 5;
constexpr int kMaxEnemyMoveInterval = 20;
constexpr int kLevelMoveIntervalDivisor = 9;
constexpr int kLevelUpTime = 10;          // 升级间隔(秒)
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

// ========== 作弊 ==========
constexpr int kCheatAddScore = 1000;
constexpr int kCheatAddBullets = 100;
constexpr int kCheatGodlikeLife = 9999999;
constexpr int kCheatGodlikeBullets = 9999999;

// ========== 显示 ==========
constexpr int kMaxLifeDisplay = 10;
constexpr int kMaxHighScores = 5;
constexpr char kBulletChar = '|';

} // namespace GameConfig