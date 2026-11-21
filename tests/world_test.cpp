// GameWorld 回归测试：锁定现有玩法规则（Stage 2 不改规则，只防回归）。
// 时间通过 Tick(input, dt) 注入，场景通过 tests/world_test_access.h 摆位。

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <string>

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

/// 清空敌机 / pending / 子弹 / 血包，便于构造无随机干扰的场景。
GameWorld MakeIdleWorld(Difficulty difficulty = Difficulty::Easy)
{
	GameWorld world;
	world.Reset(difficulty);
	WorldTestAccess::ClearEnemies(world);
	WorldTestAccess::ClearPending(world);
	WorldTestAccess::ClearBullets(world);
	WorldTestAccess::ClearPickups(world);
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
	    {Difficulty::Easy, GameConfig::kEasyLife, GameConfig::kEasyBullets, 2},
	    {Difficulty::Normal, GameConfig::kNormalLife, GameConfig::kNormalBullets, 2},
	    {Difficulty::Hard, GameConfig::kHardLife, GameConfig::kHardBullets, 4},
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
	ASSERT_GT(WorldTestAccess::EnemyCount(world), 0) << "顶行空时应从 pending 生成一批敌机";

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
		EXPECT_EQ(WorldTestAccess::LastLevelUpTime(world), GameConfig::kLevelUpTime);
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
		EXPECT_EQ(snap.player_x, GameConfig::kScreenWidth - GameConfig::kPlayerWidth);
		EXPECT_EQ(snap.player_y, GameConfig::kScreenHeight - GameConfig::kPlayerHeight);
	}
}

// ===================================================================
// 子弹
// ===================================================================

TEST(WorldBullet, HitsEnemyConsumesBulletAndRewards)
{
	GameWorld world = MakeIdleWorld();
	const WorldSnapshot start = world.Snapshot();
	const int player_y = start.player_y;
	const int bullet_x = start.player_x + 1;
	WorldTestAccess::AddEnemy(world, bullet_x, player_y - 8, 0); // 玩家上方 8 行

	GameInput fire;
	fire.fire = true;
	world.Tick(fire, kFrame);

	ASSERT_EQ(WorldTestAccess::BulletCount(world), 1);
	const WorldSnapshot after_fire = world.Snapshot();
	ASSERT_EQ(after_fire.bullets.size(), 1u);
	EXPECT_EQ(after_fire.bullets[0].x, bullet_x);
	EXPECT_EQ(after_fire.bullets[0].y, player_y - 2); // 开火同一帧内已上移一格
	EXPECT_EQ(after_fire.bullets_remaining, GameConfig::kEasyBullets - 1);

	for(int i = 0; i < 6; ++i)
		world.Tick(GameInput{}, kFrame);

	const WorldSnapshot snap = world.Snapshot();
	EXPECT_EQ(WorldTestAccess::BulletCount(world), 0);
	EXPECT_EQ(WorldTestAccess::EnemyCount(world), 0);
	EXPECT_EQ(snap.score, GameConfig::kScorePerKill);
	EXPECT_EQ(snap.bullets_remaining, GameConfig::kEasyBullets - 1 + GameConfig::kBulletsPerKill);
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
	EXPECT_EQ(snap.bullets_remaining, GameConfig::kEasyBullets + GameConfig::kBulletsPerLevelUp);
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
	EXPECT_EQ(WorldTestAccess::EnemyMoveInterval(world), GameConfig::kMinEnemyMoveInterval);
}

// ===================================================================
// 计分
// ===================================================================

TEST(WorldScore, GainsOncePerElapsedSecond)
{
	GameWorld world = MakeIdleWorld();
	for(int i = 0; i < 10; ++i)
		world.Tick(GameInput{}, seconds(1));

	EXPECT_EQ(world.Snapshot().score, 10 * GameConfig::kScorePerSecond);
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

// ===================================================================
// 掉落物（血包 / 弹药包）
// ===================================================================

namespace
{

/// 掉落物按固定间隔下落：推进恰好一个间隔让所有掉落物移动一步。
void StepOnePickupMove(GameWorld& world)
{
	for(int i = 0; i < GameConfig::kPickupMoveInterval; ++i)
		world.Tick(GameInput{}, kFrame);
}

} // namespace

TEST(WorldPickup, FallsAndIsExposedInSnapshot)
{
	GameWorld world = MakeIdleWorld();
	WorldTestAccess::AddPickup(world, 10, 10);

	const WorldSnapshot start = world.Snapshot();
	ASSERT_EQ(start.pickups.size(), 1u);
	EXPECT_EQ(start.pickups[0].x, 10);
	EXPECT_EQ(start.pickups[0].y, 10);
	EXPECT_EQ(start.pickups[0].width, GameConfig::kPickupSize);
	EXPECT_EQ(start.pickups[0].height, GameConfig::kPickupSize);
	EXPECT_EQ(start.pickups[0].kind, PickupKind::Health);

	// 间隔未到之前不动
	world.Tick(GameInput{}, kFrame);
	ASSERT_EQ(world.Snapshot().pickups.size(), 1u);
	EXPECT_EQ(world.Snapshot().pickups[0].y, 10);

	StepOnePickupMove(world);

	const WorldSnapshot after = world.Snapshot();
	ASSERT_EQ(after.pickups.size(), 1u);
	EXPECT_EQ(after.pickups[0].y, 10 + GameConfig::kPickupSpeed);
}

// 下落节奏必须与敌机同一数量级：
// 48 格 / (间隔 / 62.5 帧每秒) 应落在十几秒，而不是零点几秒。
TEST(WorldPickup, FallSpeedIsComparableToEnemies)
{
	const double ticks_per_second = 1000.0 / GameConfig::kFrameDelayMs;
	const double seconds_to_cross =
	    GameConfig::kScreenHeight * GameConfig::kPickupMoveInterval / ticks_per_second;

	EXPECT_GT(seconds_to_cross, 5.0) << "太快：会像瞬移一样掉下来";
	EXPECT_LT(seconds_to_cross, 40.0) << "太慢：掉落物会长时间占屏";
}

TEST(WorldPickup, HealthRestoresLifeAndIsConsumed)
{
	GameWorld world = MakeIdleWorld();
	const WorldSnapshot start = world.Snapshot();
	const int life_before = start.life;

	WorldTestAccess::AddPickup(world, start.player_x, start.player_y - 1, PickupKind::Health);

	StepOnePickupMove(world);

	const WorldSnapshot after = world.Snapshot();
	EXPECT_EQ(after.pickups.size(), 0u) << "拾取后应消失";
	EXPECT_EQ(after.life, life_before + GameConfig::kHealthPickupLife);
	EXPECT_EQ(after.bullets_remaining, start.bullets_remaining) << "血包不应影响弹药";
}

TEST(WorldPickup, AmmoAddsBulletsAndIsConsumed)
{
	GameWorld world = MakeIdleWorld();
	const WorldSnapshot start = world.Snapshot();

	WorldTestAccess::AddPickup(world, start.player_x, start.player_y - 1, PickupKind::Ammo);

	StepOnePickupMove(world);

	const WorldSnapshot after = world.Snapshot();
	EXPECT_EQ(after.pickups.size(), 0u) << "拾取后应消失";
	EXPECT_EQ(after.bullets_remaining, start.bullets_remaining + GameConfig::kAmmoPickupBullets);
	EXPECT_EQ(after.life, start.life) << "弹药包不应加命";
}

TEST(WorldPickup, NoCollectWhenNotOverlapping)
{
	GameWorld world = MakeIdleWorld();
	const WorldSnapshot start = world.Snapshot();

	// 玩家默认在最底行，道具很快就掉出屏幕；先把玩家挪到中间
	WorldTestAccess::SetPlayer(world, start.player_x, 30);
	WorldTestAccess::AddPickup(world, start.player_x + 6, 29, PickupKind::Ammo);

	StepOnePickupMove(world);

	EXPECT_EQ(world.Snapshot().life, start.life);
	EXPECT_EQ(world.Snapshot().bullets_remaining, start.bullets_remaining);
	EXPECT_EQ(WorldTestAccess::PickupCount(world), 1);
}

TEST(WorldPickup, RemovedAfterLeavingScreen)
{
	GameWorld world = MakeIdleWorld();
	WorldTestAccess::AddPickup(world, 5, GameConfig::kScreenHeight - 1);

	StepOnePickupMove(world);

	EXPECT_EQ(WorldTestAccess::PickupCount(world), 0);
	EXPECT_EQ(world.Snapshot().life, GameConfig::kEasyLife) << "掉落不结算";
}

TEST(WorldPickup, ResetClearsExistingPickups)
{
	GameWorld world = MakeIdleWorld();
	WorldTestAccess::AddPickup(world, 20, 20, PickupKind::Ammo);
	ASSERT_EQ(WorldTestAccess::PickupCount(world), 1);

	world.Reset(Difficulty::Normal);

	EXPECT_EQ(WorldTestAccess::PickupCount(world), 0);
}

TEST(WorldPickup, DropsBothKindsWhenEnemyIsKilled)
{
	GameWorld world(20260101);
	world.Reset(Difficulty::Easy);
	WorldTestAccess::ClearEnemies(world);
	WorldTestAccess::ClearPending(world);
	WorldTestAccess::ClearBullets(world);
	WorldTestAccess::ClearPickups(world);

	int health = 0;
	int ammo = 0;
	for(int i = 0; i < 400; ++i)
	{
		// 敌机固定在 (10,20)，子弹从下一格上移正好命中
		WorldTestAccess::AddEnemy(world, 10, 20, 0);
		WorldTestAccess::AddBullet(world, 10, 21);
		world.Tick(GameInput{}, kFrame);

		for(int k = 0; k < WorldTestAccess::PickupCount(world); ++k)
		{
			if(WorldTestAccess::PickupKindAt(world, k) == PickupKind::Ammo)
				++ammo;
			else
				++health;
		}

		WorldTestAccess::ClearPickups(world);
		WorldTestAccess::ClearEnemies(world);
		WorldTestAccess::ClearBullets(world);
	}

	// 两种掉落率都是 8%，400 次击杀里一次都不出几乎不可能
	EXPECT_GT(health, 0) << "应至少掉过一次血包";
	EXPECT_GT(ammo, 0) << "应至少掉过一次弹药包";
	EXPECT_LT(health + ammo, 400) << "不应该是必掉";
}
// ===================================================================
// 拾取判定必须跟"看到的身体"一致，而不是 3x1 的伤害命中框
// ===================================================================

TEST(WorldPickup, CollectsWhenTouchingVisualBodyOnly)
{
	GameWorld world = MakeIdleWorld();
	const WorldSnapshot start = world.Snapshot();

	// 把玩家放到中间。玩家身体向上悬垂 kPlayerGrabHeight 行，
	// 而 3x1 的伤害命中框只在第 30 行。
	WorldTestAccess::SetPlayer(world, start.player_x, 30);

	// 道具停在视觉身体范围内（第 27..29 行），完全不碰伤害命中框。
	WorldTestAccess::AddPickup(world, start.player_x, 27, PickupKind::Health);

	// 只推进到"下次移动之前"，保证判定发生在道具还在第 27 行的时候
	for(int i = 0; i < GameConfig::kPickupMoveInterval - 1; ++i)
		world.Tick(GameInput{}, kFrame);

	EXPECT_EQ(WorldTestAccess::PickupCount(world), 0)
	    << "压在身体上就应该捡到，不该等它掉到脚底那 1 行";
	EXPECT_EQ(world.Snapshot().life, start.life + GameConfig::kHealthPickupLife);
}

TEST(WorldPickup, CollectedWhileStandingOnBottomRow)
{
	GameWorld world = MakeIdleWorld();
	const WorldSnapshot start = world.Snapshot();
	ASSERT_EQ(start.player_y, GameConfig::kScreenHeight - 1) << "默认就在最下面一排";

	WorldTestAccess::AddPickup(world, start.player_x, 30, PickupKind::Ammo);

	const int ammo_before = start.bullets_remaining;
	bool collected = false;
	for(int i = 0; i < 500 && !collected; ++i)
	{
		world.Tick(GameInput{}, kFrame);
		collected = world.Snapshot().bullets_remaining > ammo_before;
	}

	EXPECT_TRUE(collected) << "贴底站着也必须能捡到落下来的道具";
}

// ===================================================================
// 掉落率随难度变化
// ===================================================================

namespace
{

/// 用固定方式连杀若干敌机，统计掉落总数。
int CountDrops(GameWorld& world, int kills)
{
	int drops = 0;
	for(int i = 0; i < kills; ++i)
	{
		WorldTestAccess::AddEnemy(world, 10, 20, 0);
		WorldTestAccess::AddBullet(world, 10, 21);
		world.Tick(GameInput{}, kFrame);

		drops += WorldTestAccess::PickupCount(world);

		WorldTestAccess::ClearPickups(world);
		WorldTestAccess::ClearEnemies(world);
		WorldTestAccess::ClearBullets(world);
	}
	return drops;
}

GameWorld MakeDropWorld(Difficulty difficulty)
{
	GameWorld world(97531);
	world.Reset(difficulty);
	WorldTestAccess::ClearEnemies(world);
	WorldTestAccess::ClearPending(world);
	WorldTestAccess::ClearBullets(world);
	WorldTestAccess::ClearPickups(world);
	return world;
}

} // namespace

TEST(WorldPickup, DropRatesFallWithDifficulty)
{
	GameWorld easy = MakeDropWorld(Difficulty::Easy);
	GameWorld normal = MakeDropWorld(Difficulty::Normal);
	GameWorld hard = MakeDropWorld(Difficulty::Hard);

	// Easy 10%+12%，Normal 8%+8%，Hard 6%+6%
	// 低难度是新手保护，高难度少给补给：高手不需要救济。
	const int easy_drops = CountDrops(easy, 600);
	const int normal_drops = CountDrops(normal, 600);
	const int hard_drops = CountDrops(hard, 600);

	EXPECT_LT(normal_drops, easy_drops);
	EXPECT_LT(hard_drops, normal_drops);

	// 顺带确认数量级没跑偏（Easy 期望约 132，Hard 期望约 72）
	EXPECT_GT(hard_drops, 20) << "困难也不是完全不给";
	EXPECT_LT(easy_drops, 600);
}
// ===================================================================
// 敌机之间不许互相重叠（命中框改成 3x2 之后容易出问题）
// ===================================================================

namespace
{

int OverlappingPairs(const WorldSnapshot& snap)
{
	int count = 0;
	for(std::size_t i = 0; i < snap.enemies.size(); ++i)
	{
		for(std::size_t j = i + 1; j < snap.enemies.size(); ++j)
		{
			const auto& a = snap.enemies[i];
			const auto& b = snap.enemies[j];
			const bool apart = (a.x + a.width <= b.x) || (b.x + b.width <= a.x)
			    || (a.y + a.height <= b.y) || (b.y + b.height <= a.y);
			if(!apart)
				++count;
		}
	}
	return count;
}

} // namespace

TEST(WorldEnemy, EnemiesNeverOverlapEachOther)
{
	GameWorld world(13579);
	world.Reset(Difficulty::Hard);

	int first_bad = -1;
	int bad_pairs = 0;
	for(int i = 0; i < 3000; ++i)
	{
		world.Tick(GameInput{}, kFrame);
		const int pairs = OverlappingPairs(world.Snapshot());
		if(pairs > 0)
		{
			first_bad = i;
			bad_pairs = pairs;
			break;
		}
	}

	EXPECT_EQ(first_bad, -1) << "第 " << first_bad << " 帧出现 " << bad_pairs << " 对重叠敌机";
}
// ===================================================================
// 掉落物不能叠在一起
// ===================================================================

namespace
{

/// 返回第一对互相重叠的道具（测试失败时用来定位），没有重叠时返回空串。
std::string FirstOverlappingPickupPair(const WorldSnapshot& snap)
{
	for(std::size_t i = 0; i < snap.pickups.size(); ++i)
	{
		for(std::size_t j = i + 1; j < snap.pickups.size(); ++j)
		{
			const auto& a = snap.pickups[i];
			const auto& b = snap.pickups[j];
			const bool apart = (a.x + a.width <= b.x) || (b.x + b.width <= a.x)
			    || (a.y + a.height <= b.y) || (b.y + b.height <= a.y);
			if(apart)
				continue;
			return "A(" + std::to_string(a.x) + "," + std::to_string(a.y) + ") B("
			    + std::to_string(b.x) + "," + std::to_string(b.y) + ")";
		}
	}
	return {};
}

} // namespace

TEST(WorldPickup, PickupsNeverOverlapEachOther)
{
	GameWorld world(20260101);
	world.Reset(Difficulty::Easy); // 掉率最高，最容易撞
	WorldTestAccess::ClearEnemies(world);
	WorldTestAccess::ClearPending(world);
	WorldTestAccess::ClearBullets(world);
	WorldTestAccess::ClearPickups(world);

	int overlaps = 0;
	int first_tick = -1;
	std::string first_pair;
	std::size_t most_seen = 0;
	for(int i = 0; i < 3000; ++i)
	{
		// 真实成因：同一列上下两只敌机（间隔 2 行，彼此不重叠）几乎同时被击毁，
		// 掉落的两个 3x3 道具在竖直方向就会压在一起
		if(i % 40 == 0)
		{
			const int x = 20 + (i * 13) % 40;
			WorldTestAccess::AddEnemy(world, x, 20, 0);
			WorldTestAccess::AddEnemy(world, x, 22, 0);
			WorldTestAccess::AddBullet(world, x + 1, 21);
			WorldTestAccess::AddBullet(world, x + 1, 23);
		}

		world.Tick(GameInput{}, kFrame);

		const WorldSnapshot snap = world.Snapshot();
		most_seen = std::max(most_seen, snap.pickups.size());

		const std::string pair = FirstOverlappingPickupPair(snap);
		if(pair.empty())
			continue;

		++overlaps;
		if(first_pair.empty())
		{
			first_pair = pair;
			first_tick = i;
		}
	}

	EXPECT_GE(most_seen, 3u) << "应该同时存在多个掉落物，这条测试才有意义";
	EXPECT_EQ(overlaps, 0) << "掉落物叠在一起了（第 " << first_tick << " 帧：" << first_pair << "）";
}