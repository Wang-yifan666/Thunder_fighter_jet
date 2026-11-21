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

// ========== 开火节奏 ==========
// 每 2 帧一发（约 31 发/秒）。原来是每帧一发（62 发/秒）：
// 一帧一发时"打两下才死"只差 16ms，玩家根本感觉不到，护盾和精英形同虚设。
constexpr int kFireIntervalFrames = 2;

// 火力强化：三路同时开火（x-1 / x / x+1），只消耗 1 发弹药，限时生效。
constexpr int kFirepowerDurationSec = 15;
constexpr int kFirepowerFrames = kFirepowerDurationSec * 1000 / kFrameDelayMs;

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

// 后期堆怪时的泄压阀：场上 + 排队的小怪超过阈值后，每次刷怪多放几只。
// 每多 kCrowdBatchPerStep 只加 1，最多加 kCrowdBatchMaxBonus 只。
constexpr int kCrowdBatchThreshold = 500;
constexpr int kCrowdBatchPerStep = 200;
constexpr int kCrowdBatchMaxBonus = 3;

// ========== 精英敌机 ==========
// 血量 2（带盾则 3 下），分数 2.5 倍，弹药奖励更高——它是"值得打"的目标。
constexpr int kEliteWidth = 4;
constexpr int kEliteHeight = 3;
constexpr int kEliteHp = 2;
constexpr int kScorePerEliteKill = 250;
constexpr int kBulletsPerEliteKill = 8;
constexpr int kEliteShieldPercent = 50; // 精英自带护盾的概率

// 出现概率：p(L) = cap * (1 - exp(-(L - kEliteMinLevel) / K))，到 100 级前后饱和。
constexpr int kEliteMinLevel = 8;      // 第 9 级开始可能出现
constexpr int kEliteSpawnCurveK = 15;  // 指数饱和常数，越大前期越少见
constexpr int kEliteCapEasyPercent = 40;
constexpr int kEliteCapNormalPercent = 60;
constexpr int kEliteCapHardPercent = 75;

// ========== 护盾 ==========
// 小怪护盾：5 级之后随机出现在普通小怪身上，需要先打掉盾（多打一发）。
constexpr int kEnemyShieldMinLevel = 5;
constexpr int kEasyShieldedEnemyPercent = 10;
constexpr int kNormalShieldedEnemyPercent = 15;
constexpr int kHardShieldedEnemyPercent = 20;

// 敌人破盾后无敌若干帧：子弹穿过（不消耗），敌人闪烁。
// 20 帧 ≈ 0.32 秒，正好让"破盾 → 补两发"有节奏。
constexpr int kEnemyShieldBreakInvulnFrames = 20;
// 玩家破盾沿用受伤后的 1 秒无敌（60 帧），否则 0.32 秒后会被下一只怪直接撞掉血。
constexpr int kShieldBreakSparkFrames = 6; // 命中火花的存活帧数（短一点，别像残留的子弹）

// 破盾冲击波：以玩家为中心，圆形范围内每只敌机吃 1 点伤害。
constexpr float kShockwaveRadiusCells = 10.0f;
constexpr int kShockwaveEffectFrames = 12; // 冲击波动画帧数
constexpr int kDuplicateShieldScore = 50;  // 已有护盾时再拾取护盾包的补偿分

// ========== 玩家 ==========
constexpr int kPlayerWidth = 3;
constexpr int kPlayerHeight = 1;
constexpr int kInvincibleDurationSec = 1;

// 拾取判定用的"视觉身体"高度（格）。
// 精灵是 32x32 像素、底边对齐命中框底边，肉眼看到的身体有 4 行高；
// 受伤害仍用 kPlayerHeight(1 行) 的命中框，两者必须分开：
// 拿 1 行的伤害判定去做拾取，会出现"从道具上碾过去却没吃到"。
constexpr int kPlayerGrabHeight = 4;

// ========== 掉落物（血包 / 弹药包 / 护盾包 / 火力包） ==========
constexpr int kPickupSize = 3;          // 3x3 格，精灵与判定等大
constexpr int kPickupSpeed = 1;         // 每次移动的格子数
constexpr int kPickupMoveInterval = 16; // 每多少帧移动一次（约 12 秒穿过全屏）
constexpr int kHealthPickupLife = 1;    // 血包回复的生命值
constexpr int kAmmoPickupBullets = 50;  // 弹药包补充的子弹数
// 落点被其它道具占用时，向两侧各尝试几个身位；都占满就放弃这次掉落
constexpr int kPickupPlacementTries = 4;

// 掉落率随难度变化（百分比）。一次掷骰切四个区间，先血包、再弹药、护盾、火力。
// 难度越低补给越慷慨：Easy 是新手保护，Hard 靠自己本事——高手不需要救济。
constexpr int kEasyHealthDropPercent = 4;
constexpr int kEasyAmmoDropPercent = 4;
constexpr int kEasyShieldDropPercent = 3;
constexpr int kEasyFirepowerDropPercent = 2;
constexpr int kNormalHealthDropPercent = 3;
constexpr int kNormalAmmoDropPercent = 3;
constexpr int kNormalShieldDropPercent = 2;
constexpr int kNormalFirepowerDropPercent = 1;
constexpr int kHardHealthDropPercent = 2;
constexpr int kHardAmmoDropPercent = 2;
constexpr int kHardShieldDropPercent = 2;
constexpr int kHardFirepowerDropPercent = 1;

// ========== 作弊 ==========
constexpr int kCheatAddScore = 1000;
constexpr int kCheatAddBullets = 100;
constexpr int kCheatGodlikeLife = 9999999;
constexpr int kCheatGodlikeBullets = 9999999;
constexpr int kCheatNoticeMs = 1500; // 切换作弊模式后提示显示时长

// ========== 显示 ==========
constexpr int kMaxHighScores = 5;

} // namespace GameConfig