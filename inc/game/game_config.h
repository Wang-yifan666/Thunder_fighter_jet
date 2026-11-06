#pragma once

namespace GameConfig
{

// ========== 屏幕 / 窗口 ==========
constexpr int kScreenWidth = 102;  // 原 98 列，左右各扩充 2 列
constexpr int kScreenHeight = 48;  // 原 40 行，上下各扩充 4 行
constexpr int kWindowWidth = 104;  // 棋盘 102 列 + 边框
constexpr int kWindowHeight = 58;  // 棋盘 48 行 + HUD/边框余量

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
constexpr int kEnemyHeight = 2;  // 原 1 行；精灵需要方形空间，命中框同步加高
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

// ========== 掉落物（血包 / 弹药包） ==========
constexpr int kPickupSize = 3;              // 3x3 格，精灵与判定等大
constexpr int kPickupSpeed = 1;             // 每次移动的格子数
constexpr int kPickupMoveInterval = 16;     // 每多少帧移动一次（约 12 秒穿过全屏）
constexpr int kHealthDropPercent = 8;       // 击毁敌机后掉血包的概率(%)
constexpr int kAmmoDropPercent = 8;         // 击毁敌机后掉弹药包的概率(%)
constexpr int kHealthPickupLife = 1;        // 血包回复的生命值
constexpr int kAmmoPickupBullets = 50;      // 弹药包补充的子弹数

// ========== 作弊 ==========
constexpr int kCheatAddScore = 1000;
constexpr int kCheatAddBullets = 100;
constexpr int kCheatGodlikeLife = 9999999;
constexpr int kCheatGodlikeBullets = 9999999;
constexpr int kCheatNoticeMs = 1500;  // 切换作弊模式后提示显示时长

// ========== 显示 ==========
constexpr int kMaxLifeDisplay = 10;
constexpr int kMaxHighScores = 5;
constexpr char kBulletChar = '|';
constexpr char kHealthChar = '+';
constexpr char kAmmoChar = '@';

} // namespace GameConfig