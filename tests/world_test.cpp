// GameWorld 回归测试：锁定现有玩法规则（Stage 2 不改规则，只防回归）。
// 时间通过 Tick(input, dt) 注入，场景通过 tests/world_test_access.h 摆位。

#include <gtest/gtest.h>

#include <chrono>

#include "game/game_config.h"
#include "game/game_types.h"
#include "game/game_world.h"
#include "world_test_access.h"

using namespace std::chrono;
using thunder::GameInput;
using thunder::GameWorld;
using thunder::WorldSnapshot;
using thunder::WorldTestAccess;

namespace
{

/// 清空敌机 / pending / 子弹，便于构造无随机干扰的场景。
GameWorld MakeIdleWorld(Difficulty difficulty = Difficulty::Easy)
{
	GameWorld world;
	world.Reset(difficulty);
	WorldTestAccess::ClearEnemies(world);
	WorldTestAccess::ClearPending(world);
	WorldTestAccess::ClearBullets(world);
	return world;
}

constexpr auto kFrame = milliseconds(16);

} // namespace

// ===================================================================
// 重置
// ===================================================================

TEST(WorldReset, DifficultyInitialValues)
{
	struct Case
	{
		Difficulty difficulty;
		int life;
		int ammo;
		int pending;
	};

	const Case cases[] = {
	    {Difficulty::Easy, GameConfig::kEasyLife, GameConfig::kEasyBullets,
	     2},
	    {Difficulty::Normal, GameConfig::kNormalLife,
	     GameConfig::kNormalBullets, 2},
	    {Difficulty::Hard, GameConfig::kHardLife, GameConfig::kHardBullets,
	     4},
	};

	for(const auto& c: cases)
	{
		GameWorld world;
		world.Reset(c.difficulty);
		const WorldSnapshot snap = world.Snapshot();

		EXPECT_EQ(snap.life, c.life);
		EXPECT_EQ(snap.bullets_remaining, c.ammo);
		EXPECT_EQ(snap.pending_enemies, c.pending);
		EXPECT_EQ(snap.level, 1);
		EXPECT_EQ(snap.score, 0);
		EXPECT_EQ(snap.elapsed_seconds, 0);
		EXPECT_TRUE(snap.enemies.empty());
		EXPECT_TRUE(snap.bullets.empty());
		EXPECT_EQ(snap.player_x, GameConfig::kScreenWidth / 2 - 1);
		EXPECT_EQ(snap.player_y, GameConfig::kScreenHeight - 1);
		EXPECT_FALSE(snap.over);
	}
}

TEST(WorldReset, ClearsActiveEnemiesAndRebuildsPending)
{
	GameWorld world;
	world.Reset(Difficulty::Easy);

	GameInput none;
	world.Tick(none, kFrame);
	ASSERT_GT(WorldTestAccess::EnemyCount(world), 0)
	    << "顶行空时应从 pending 生成一批敌机";

	GameInput add_score;
	add_score.digit[4] = true;
	world.Tick(add_score, kFrame);
	ASSERT_GT(world.Score(), 0);

	world.Reset(Difficulty::Normal);

	// 活动敌机清空，pending 按难度重建（Normal 1 级 = 2）
	EXPECT_EQ(WorldTestAccess::EnemyCount(world), 0);
	EXPECT_EQ(WorldTestAccess::PendingCount(world), 2);

	const WorldSnapshot snap = world.Snapshot();
	EXPECT_EQ(snap.life, GameConfig::kNormalLife);
	EXPECT_EQ(snap.bullets_remaining, GameConfig::kNormalBullets);
	EXPECT_EQ(snap.score, 0);
	EXPECT_EQ(snap.level, 1);
	EXPECT_EQ(snap.elapsed_seconds, 0);
}

TEST(WorldReset, LevelUpTimerIsPerGame)
{
	GameInput none;

	for(int round = 0; round < 2; ++round)
	{
		GameWorld world = MakeIdleWorld();
		for(int i = 0; i < GameConfig::kLevelUpTime; ++i)
			world.Tick(none, seconds(1));

		EXPECT_EQ(world.Snapshot().level, 2) << "round " << round;
		EXPECT_EQ(WorldTestAccess::LastLevelUpTime(world),
		          GameConfig::kLevelUpTime);
	}
}

// ===================================================================
// 玩家边界
// ===================================================================

TEST(WorldPlayer, ClampsToScreenBounds)
{
	{
		GameWorld world = MakeIdleWorld();
		GameInput left;
		left.left = true;
		for(int i = 0; i < 200; ++i)
			world.Tick(left, kFrame);
		EXPECT_EQ(world.Snapshot().player_x, 0);
	}

	{
		GameWorld world = MakeIdleWorld();
		GameInput up;
		up.up = true;
		for(int i = 0; i < 400; ++i)
			world.Tick(up, kFrame);
		EXPECT_EQ(world.Snapshot().player_y, 0);
	}

	{
		GameWorld world = MakeIdleWorld();
		GameInput right_down;
		right_down.right = true;
		right_down.down = true;
		for(int i = 0; i < 400; ++i)
			world.Tick(right_down, kFrame);

		const WorldSnapshot snap = world.Snapshot();
		EXPECT_EQ(snap.player_x,
		          GameConfig::kScreenWidth - GameConfig::kPlayerWidth);
		EXPECT_EQ(snap.player_y,
		          GameConfig::kScreenHeight - GameConfig::kPlayerHeight);
	}
}

// ===================================================================
// 子弹
// ===================================================================

TEST(WorldBullet, HitsEnemyConsumesBulletAndRewards)
{
	GameWorld world = MakeIdleWorld();
	const int player_y = world.Snapshot().player_y;
	WorldTestAccess::AddEnemy(world, 49, player_y - 8, 0); // 停在玩家上方 8 行

	GameInput fire;
	fire.fire = true;
	world.Tick(fire, kFrame);

	ASSERT_EQ(WorldTestAccess::BulletCount(world), 1);
	const WorldSnapshot after_fire = world.Snapshot();
	ASSERT_EQ(after_fire.bullets.size(), 1u);
	EXPECT_EQ(after_fire.bullets[0].x, 49);
	EXPECT_EQ(after_fire.bullets[0].y, player_y - 2); // 开火同一帧内已上移一格
	EXPECT_EQ(after_fire.bullets_remaining,
	          GameConfig::kEasyBullets - 1);

	for(int i = 0; i < 6; ++i)
		world.Tick(GameInput{}, kFrame);

	const WorldSnapshot snap = world.Snapshot();
	EXPECT_EQ(WorldTestAccess::BulletCount(world), 0);
	EXPECT_EQ(WorldTestAccess::EnemyCount(world), 0);
	EXPECT_EQ(snap.score, GameConfig::kScorePerKill);
	EXPECT_EQ(snap.bullets_remaining, GameConfig::kEasyBullets - 1
	                                      + GameConfig::kBulletsPerKill);
}

TEST(WorldBullet, ExpiresAboveScreenWithoutScore)
{
	GameWorld world = MakeIdleWorld();

	GameInput fire;
	fire.fire = true;
	world.Tick(fire, kFrame);
	ASSERT_EQ(WorldTestAccess::BulletCount(world), 1);
	const int ammo_after_fire = world.Snapshot().bullets_remaining;

	for(int i = 0; i < 60; ++i)
		world.Tick(GameInput{}, kFrame);

	EXPECT_EQ(WorldTestAccess::BulletCount(world), 0);
	EXPECT_EQ(world.Snapshot().score, 0);
	EXPECT_EQ(world.Snapshot().bullets_remaining, ammo_after_fire);
}

// ===================================================================
// 无敌时间
// ===================================================================

TEST(WorldInvincible, BlocksRepeatedDamageForOneSecond)
{
	GameWorld world = MakeIdleWorld();
	const WorldSnapshot start = world.Snapshot();
	WorldTestAccess::AddEnemy(world, start.player_x, start.player_y, 0);

	world.Tick(GameInput{}, kFrame);
	ASSERT_EQ(world.Snapshot().life, GameConfig::kEasyLife - 1);
	ASSERT_TRUE(WorldTestAccess::IsInvincible(world));

	// 800ms 内不应再掉血
	for(int i = 0; i < 50; ++i)
		world.Tick(GameInput{}, kFrame);
	EXPECT_EQ(world.Snapshot().life, GameConfig::kEasyLife - 1);

	// 累计 1120ms，无敌结束后的接触应再次掉血
	for(int i = 0; i < 20; ++i)
		world.Tick(GameInput{}, kFrame);
	EXPECT_EQ(world.Snapshot().life, GameConfig::kEasyLife - 2);
}

TEST(WorldInvincible, CheatInvincibilityPreventsAllDamage)
{
	GameWorld world = MakeIdleWorld();
	const WorldSnapshot start = world.Snapshot();
	WorldTestAccess::AddEnemy(world, start.player_x, start.player_y, 0);

	GameInput cheat;
	cheat.digit[5] = true;
	world.Tick(cheat, kFrame);
	ASSERT_TRUE(WorldTestAccess::CheatInvincible(world));

	for(int i = 0; i < 200; ++i)
		world.Tick(GameInput{}, kFrame);

	EXPECT_EQ(world.Snapshot().life, GameConfig::kEasyLife);
	EXPECT_TRUE(world.Snapshot().invincible);
}

// ===================================================================
// 等级升级
// ===================================================================

TEST(WorldLevel, LevelsUpAtTenSeconds)
{
	GameWorld world = MakeIdleWorld();

	GameInput none;
	for(int i = 0; i < GameConfig::kLevelUpTime; ++i)
		world.Tick(none, seconds(1));

	const WorldSnapshot snap = world.Snapshot();
	EXPECT_EQ(snap.level, 2);
	EXPECT_EQ(snap.bullets_remaining,
	          GameConfig::kEasyBullets + GameConfig::kBulletsPerLevelUp);
	EXPECT_EQ(WorldTestAccess::EnemyMoveInterval(world), 19);
	EXPECT_EQ(WorldTestAccess::PendingCount(world), 2);
}

TEST(WorldLevel, SingleLargeDeltaLevelsOnlyOnce)
{
	GameWorld world = MakeIdleWorld();

	world.Tick(GameInput{}, seconds(30)); // 现状语义：一帧最多升一级
	EXPECT_EQ(world.Snapshot().level, 2);
	EXPECT_EQ(WorldTestAccess::LastLevelUpTime(world), 30);

	world.Tick(GameInput{}, seconds(1));
	EXPECT_EQ(world.Snapshot().level, 2);

	world.Tick(GameInput{}, seconds(10)); // 41 - 30 >= 10
	EXPECT_EQ(world.Snapshot().level, 3);
}

TEST(WorldLevel, MoveIntervalHasFloor)
{
	GameWorld world = MakeIdleWorld();

	for(int i = 0; i < 9; ++i)
		world.Tick(GameInput{}, seconds(GameConfig::kLevelUpTime));

	EXPECT_EQ(world.Snapshot().level, 10);
	EXPECT_EQ(WorldTestAccess::EnemyMoveInterval(world),
	          GameConfig::kMinEnemyMoveInterval);
}

// ===================================================================
// 计分
// ===================================================================

TEST(WorldScore, GainsOncePerElapsedSecond)
{
	GameWorld world = MakeIdleWorld();
	for(int i = 0; i < 10; ++i)
		world.Tick(GameInput{}, seconds(1));

	EXPECT_EQ(world.Snapshot().score,
	          10 * GameConfig::kScorePerSecond);
}

TEST(WorldScore, SingleLargeDeltaGainsOnlyOnce)
{
	GameWorld world = MakeIdleWorld();

	world.Tick(GameInput{}, seconds(5));

	// 现状语义：不按跨过的秒数补偿
	EXPECT_EQ(world.Snapshot().score, GameConfig::kScorePerSecond);
	EXPECT_EQ(WorldTestAccess::LastScoreTime(world), 5);
}

TEST(WorldScore, SubSecondTicksGainNothing)
{
	GameWorld world = MakeIdleWorld();
	for(int i = 0; i < 9; ++i)
		world.Tick(GameInput{}, milliseconds(100));

	EXPECT_EQ(world.Snapshot().score, 0);
}

// ===================================================================
// 结束条件
// ===================================================================

TEST(WorldGameOver, SelfKillMarksWorldOver)
{
	GameWorld world = MakeIdleWorld();

	GameInput suicide;
	suicide.digit[2] = true;
	world.Tick(suicide, kFrame);

	EXPECT_TRUE(world.Over());
	EXPECT_EQ(world.Snapshot().life, 0);
	EXPECT_TRUE(world.Snapshot().over);
}
