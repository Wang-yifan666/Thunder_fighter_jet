// 新战斗机制回归测试：射速节奏、火力强化、敌人护盾与多段血、精英、玩家护盾与冲击波。
#include <gtest/gtest.h>

#include <chrono>

#include "game/game_config.h"
#include "game/game_types.h"
#include "game/game_world.h"
#include "world_test_access.h"

using namespace std::chrono;
using thunder::GameWorld;
using thunder::WorldSnapshot;
using thunder::WorldTestAccess;

namespace
{

constexpr auto kFrame = milliseconds(GameConfig::kFrameDelayMs);

/// 清干净的一局：不刷怪、无子弹、无道具，方便手工摆位。
GameWorld MakeQuietWorld(Difficulty difficulty = Difficulty::Easy)
{
	GameWorld world(20260101);
	world.Reset(difficulty);
	WorldTestAccess::ClearEnemies(world);
	WorldTestAccess::ClearPending(world);
	WorldTestAccess::ClearBullets(world);
	WorldTestAccess::ClearPickups(world);
	WorldTestAccess::SetPlayer(world, 50, GameConfig::kScreenHeight - 1);
	return world;
}

} // namespace

// ============ 射速节奏 ============

TEST(WorldFire, FiresEveryOtherFrame)
{
	GameWorld world = MakeQuietWorld();

	thunder::GameInput fire;
	fire.fire = true;

	int shots = 0;
	int last = 0;
	for(int i = 0; i < 10; ++i)
	{
		world.Tick(fire, kFrame);
		const int now = WorldTestAccess::BulletCount(world);
		// 子弹会一路飞出屏幕外，这里只看"有没有增加"
		if(now > last)
			++shots;
		last = now;
	}

	// 10 帧、每 2 帧一发 => 5 发
	EXPECT_EQ(shots, 5) << "射速应该是每 2 帧一发";
}

TEST(WorldFire, FireCooldownCountsDownWhenNotFiring)
{
	GameWorld world = MakeQuietWorld();

	thunder::GameInput fire;
	fire.fire = true;
	world.Tick(fire, kFrame);
	EXPECT_EQ(WorldTestAccess::FireCooldown(world), GameConfig::kFireIntervalFrames);

	world.Tick(thunder::GameInput{}, kFrame);
	EXPECT_EQ(WorldTestAccess::FireCooldown(world), GameConfig::kFireIntervalFrames - 1);
}

// ============ 火力强化 ============

TEST(WorldFire, FirepowerPickupAddsTwoExtraBulletsForOneAmmo)
{
	GameWorld world = MakeQuietWorld();
	WorldTestAccess::AddPickup(world, 50, GameConfig::kScreenHeight - 1,
	                           PickupKind::Firepower);
	world.Tick(thunder::GameInput{}, kFrame); // 拾取
	EXPECT_GT(WorldTestAccess::FirepowerFramesLeft(world), 0);

	WorldTestAccess::ClearBullets(world);
	const int ammo_before = world.Snapshot().bullets_remaining;

	thunder::GameInput fire;
	fire.fire = true;
	world.Tick(fire, kFrame);

	EXPECT_EQ(WorldTestAccess::BulletCount(world), 3) << "三路开火";
	EXPECT_EQ(world.Snapshot().bullets_remaining, ammo_before - 1) << "只消耗 1 发弹药";
}

TEST(WorldFire, FirepowerExpires)
{
	GameWorld world = MakeQuietWorld();
	WorldTestAccess::AddPickup(world, 50, GameConfig::kScreenHeight - 1,
	                           PickupKind::Firepower);
	world.Tick(thunder::GameInput{}, kFrame);

	const int frames = WorldTestAccess::FirepowerFramesLeft(world);
	ASSERT_GT(frames, 0);

	// 跑满时长（每次 Tick 让 dt 正好一帧，计时才不会漂）
	for(int i = 0; i < frames; ++i)
		world.Tick(thunder::GameInput{}, kFrame);

	EXPECT_EQ(WorldTestAccess::FirepowerFramesLeft(world), 0);
	EXPECT_EQ(world.Snapshot().firepower_seconds_left, 0);
}

// ============ 敌人血量与护盾 ============

TEST(WorldEnemy, EliteTakesTwoHits)
{
	GameWorld world = MakeQuietWorld();
	WorldTestAccess::AddElite(world, 20, 10, 0);

	WorldTestAccess::AddBullet(world, 21, 11);
	world.Tick(thunder::GameInput{}, kFrame);
	EXPECT_EQ(WorldTestAccess::EnemyCount(world), 1) << "第一下打不死精英";
	EXPECT_EQ(WorldTestAccess::EnemyAt(world, 0).hp, GameConfig::kEliteHp - 1);

	WorldTestAccess::AddBullet(world, 21, 11);
	world.Tick(thunder::GameInput{}, kFrame);
	EXPECT_EQ(WorldTestAccess::EnemyCount(world), 0) << "第二下击毁";
}

TEST(WorldEnemy, ShieldedEliteTakesThreeHits)
{
	GameWorld world = MakeQuietWorld();
	WorldTestAccess::AddElite(world, 20, 10, 0, true);
	ASSERT_TRUE(WorldTestAccess::EnemyAt(world, 0).has_shield);

	WorldTestAccess::AddBullet(world, 21, 11);
	world.Tick(thunder::GameInput{}, kFrame);
	ASSERT_EQ(WorldTestAccess::EnemyCount(world), 1);
	EXPECT_FALSE(WorldTestAccess::EnemyAt(world, 0).has_shield) << "第一发破盾";
	EXPECT_EQ(WorldTestAccess::EnemyAt(world, 0).hp, GameConfig::kEliteHp) << "破盾不掉血";
	EXPECT_TRUE(WorldTestAccess::EnemyAt(world, 0).invuln_frames > 0);

	// 无敌期内打不中（子弹穿过）
	WorldTestAccess::AddBullet(world, 21, 11);
	world.Tick(thunder::GameInput{}, kFrame);
	EXPECT_EQ(WorldTestAccess::EnemyAt(world, 0).hp, GameConfig::kEliteHp);

	// 等无敌结束再补两发
	for(int i = 0; i < GameConfig::kEnemyShieldBreakInvulnFrames; ++i)
		world.Tick(thunder::GameInput{}, kFrame);

	WorldTestAccess::AddBullet(world, 21, 11);
	world.Tick(thunder::GameInput{}, kFrame);
	EXPECT_EQ(WorldTestAccess::EnemyAt(world, 0).hp, GameConfig::kEliteHp - 1);

	WorldTestAccess::AddBullet(world, 21, 11);
	world.Tick(thunder::GameInput{}, kFrame);
	EXPECT_EQ(WorldTestAccess::EnemyCount(world), 0);
}

TEST(WorldEnemy, BulletPassesThroughInvulnerableEnemy)
{
	GameWorld world = MakeQuietWorld();
	WorldTestAccess::AddElite(world, 20, 10, 0, true);

	// 破盾
	WorldTestAccess::AddBullet(world, 21, 11);
	world.Tick(thunder::GameInput{}, kFrame);
	ASSERT_TRUE(WorldTestAccess::EnemyAt(world, 0).invuln_frames > 0);

	// 再打一发：子弹不该被吃掉
	WorldTestAccess::AddBullet(world, 21, 11);
	world.Tick(thunder::GameInput{}, kFrame);
	EXPECT_EQ(WorldTestAccess::BulletCount(world), 1) << "无敌期子弹穿过，不消耗";
}

TEST(WorldEnemy, ShieldedNormalEnemyTakesTwoHits)
{
	GameWorld world = MakeQuietWorld();
	WorldTestAccess::AddShieldedEnemy(world, 20, 10);
	ASSERT_TRUE(WorldTestAccess::EnemyAt(world, 0).has_shield);

	WorldTestAccess::AddBullet(world, 21, 11);
	world.Tick(thunder::GameInput{}, kFrame);
	ASSERT_EQ(WorldTestAccess::EnemyCount(world), 1);
	EXPECT_FALSE(WorldTestAccess::EnemyAt(world, 0).has_shield) << "第一发破盾";
	EXPECT_EQ(WorldTestAccess::EnemyAt(world, 0).hp, 1) << "破盾不掉血";

	for(int i = 0; i < GameConfig::kEnemyShieldBreakInvulnFrames; ++i)
		world.Tick(thunder::GameInput{}, kFrame);

	WorldTestAccess::AddBullet(world, 21, 11);
	world.Tick(thunder::GameInput{}, kFrame);
	EXPECT_EQ(WorldTestAccess::EnemyCount(world), 0) << "破盾后再一发就死";
}

TEST(WorldEnemy, NormalEnemiesGetShieldsOnlyAboveLevel5)
{
	GameWorld world = MakeQuietWorld(Difficulty::Normal);
	WorldTestAccess::ClearPending(world);

	// 阈值等级（5 级）还不带盾
	WorldTestAccess::SetLevel(world, GameConfig::kEnemyShieldMinLevel);
	WorldTestAccess::SpawnWave(world);
	EXPECT_GT(WorldTestAccess::PendingCount(world), 0);
	EXPECT_EQ(WorldTestAccess::ShieldedNormalPendingCount(world), 0)
	    << "5 级及以下的小怪不该带盾";

	// 高等级下多刷几波，应该能抽到带盾的小怪
	WorldTestAccess::ClearPending(world);
	WorldTestAccess::SetLevel(world, 30);
	for(int i = 0; i < 20; ++i)
		WorldTestAccess::SpawnWave(world);
	EXPECT_GT(WorldTestAccess::ShieldedNormalPendingCount(world), 0)
	    << "6 级之后普通小怪会随机带盾";
}

TEST(WorldEnemy, FastEnemyCannotTunnelThroughBullet)
{
	GameWorld world = MakeQuietWorld();

	// 速度 3 的敌机：本帧会从 10..11 直接跳到 13..14，
	// 而子弹从 12 上行到 11 —— 只看末位置两者永不重叠，必须按扫掠判定算命中。
	WorldTestAccess::AddEnemy(world, 20, 10, 3, GameConfig::kEnemyWidth,
	                          GameConfig::kEnemyHeight);
	WorldTestAccess::AddBullet(world, 21, 12);

	// 让下一帧正好是敌机的移动帧
	WorldTestAccess::SetEnemyMoveCounter(world, WorldTestAccess::EnemyMoveInterval(world) - 1);
	world.Tick(thunder::GameInput{}, kFrame);

	EXPECT_EQ(WorldTestAccess::EnemyCount(world), 0) << "高速敌机不该从子弹身上跨过去";
	EXPECT_EQ(WorldTestAccess::BulletCount(world), 0) << "这一发要算命中并被消耗";
}

TEST(WorldEnemy, ShotTwoRowsBelowStillMisses)
{
	// 反向保护：敌机不动时，下方两格的子弹这一帧只会走到下方一格，不该算命中
	GameWorld world = MakeQuietWorld();
	WorldTestAccess::AddEnemy(world, 20, 10, 0, GameConfig::kEnemyWidth,
	                          GameConfig::kEnemyHeight);
	WorldTestAccess::AddBullet(world, 21, 13);
	WorldTestAccess::SetEnemyMoveCounter(world, 0);

	world.Tick(thunder::GameInput{}, kFrame);

	EXPECT_EQ(WorldTestAccess::EnemyCount(world), 1) << "扫掠判定不该产生假命中";
}

TEST(WorldEnemy, EliteScoresTwoAndHalfTimesAndPaysMoreAmmo)
{
	GameWorld world = MakeQuietWorld();
	const int score_before = world.Score();
	const int ammo_before = world.Snapshot().bullets_remaining;

	WorldTestAccess::AddElite(world, 20, 10, 0);
	WorldTestAccess::AddBullet(world, 21, 11);
	world.Tick(thunder::GameInput{}, kFrame);
	WorldTestAccess::AddBullet(world, 21, 11);
	world.Tick(thunder::GameInput{}, kFrame);
	ASSERT_EQ(WorldTestAccess::EnemyCount(world), 0);

	const int gained = world.Score() - score_before;
	EXPECT_GE(gained, GameConfig::kScorePerEliteKill) << "精英分数 2.5 倍";
	EXPECT_GE(world.Snapshot().bullets_remaining - ammo_before, GameConfig::kBulletsPerEliteKill);
}

TEST(WorldEnemy, EliteSpawnPercentFollowsCurveAndCaps)
{
	GameWorld world = MakeQuietWorld(Difficulty::Normal);

	WorldTestAccess::SetLevel(world, GameConfig::kEliteMinLevel);
	EXPECT_EQ(WorldTestAccess::EliteSpawnPercent(world), 0) << "阈值等级不出精英";

	WorldTestAccess::SetLevel(world, 20);
	const int at20 = WorldTestAccess::EliteSpawnPercent(world);
	EXPECT_GT(at20, 0);
	EXPECT_LT(at20, GameConfig::kEliteCapNormalPercent);

	WorldTestAccess::SetLevel(world, 100);
	const int at100 = WorldTestAccess::EliteSpawnPercent(world);
	EXPECT_NEAR(at100, GameConfig::kEliteCapNormalPercent, 1) << "100 级顶到上限";

	WorldTestAccess::SetLevel(world, 200);
	EXPECT_EQ(WorldTestAccess::EliteSpawnPercent(world), at100) << "封顶后不再提高";
}

TEST(WorldEnemy, CrowdBatchBonusOnlyAboveThreshold)
{
	EXPECT_EQ(WorldTestAccess::CrowdBatchBonus(GameConfig::kCrowdBatchThreshold), 0);
	EXPECT_EQ(WorldTestAccess::CrowdBatchBonus(GameConfig::kCrowdBatchThreshold + 1), 1);
	EXPECT_EQ(WorldTestAccess::CrowdBatchBonus(GameConfig::kCrowdBatchThreshold + 150), 1);
	EXPECT_EQ(WorldTestAccess::CrowdBatchBonus(GameConfig::kCrowdBatchThreshold + 250), 2);

	const int huge = GameConfig::kCrowdBatchThreshold + 5000;
	EXPECT_EQ(WorldTestAccess::CrowdBatchBonus(huge), GameConfig::kCrowdBatchMaxBonus)
	    << "再拥堵也不会无限加";
}

// ============ 玩家护盾与冲击波 ============

TEST(WorldShield, PickupGrantsShieldAndDoesNotStack)
{
	GameWorld world = MakeQuietWorld();
	const int score_before = world.Score();

	WorldTestAccess::AddPickup(world, 50, GameConfig::kScreenHeight - 1,
	                           PickupKind::Shield);
	world.Tick(thunder::GameInput{}, kFrame);
	EXPECT_TRUE(WorldTestAccess::PlayerShield(world));

	WorldTestAccess::AddPickup(world, 50, GameConfig::kScreenHeight - 1,
	                           PickupKind::Shield);
	world.Tick(thunder::GameInput{}, kFrame);
	EXPECT_TRUE(WorldTestAccess::PlayerShield(world)) << "护盾不能叠层";
	EXPECT_GE(world.Score() - score_before, GameConfig::kDuplicateShieldScore);
}

TEST(WorldShield, ShieldAbsorbsHitAndShockwaveClearsNearby)
{
	GameWorld world = MakeQuietWorld();
	WorldTestAccess::AddPickup(world, 50, GameConfig::kScreenHeight - 1,
	                           PickupKind::Shield);
	world.Tick(thunder::GameInput{}, kFrame);
	ASSERT_TRUE(WorldTestAccess::PlayerShield(world));

	const int life_before = world.Snapshot().life;

	// 玩家正上方摆一只小怪：破盾冲击波应该把它清掉
	WorldTestAccess::AddEnemy(world, 50, GameConfig::kScreenHeight - 1, 0);
	world.Tick(thunder::GameInput{}, kFrame);

	EXPECT_EQ(world.Snapshot().life, life_before) << "护盾挡下这次撞击，不掉血";
	EXPECT_FALSE(WorldTestAccess::PlayerShield(world));
	EXPECT_EQ(WorldTestAccess::EnemyCount(world), 0) << "冲击波清掉了贴脸的敌机";
	EXPECT_TRUE(WorldTestAccess::IsInvincible(world)) << "破盾后照样进入无敌时间";
	EXPECT_GT(WorldTestAccess::EffectCount(world), 0) << "有冲击波特效";
}

TEST(WorldShield, ShockwaveRespectsRadius)
{
	GameWorld world = MakeQuietWorld();
	WorldTestAccess::AddPickup(world, 50, GameConfig::kScreenHeight - 1,
	                           PickupKind::Shield);
	world.Tick(thunder::GameInput{}, kFrame);

	// 远处（超过冲击波半径）的小怪不该被波及
	const int far_y = GameConfig::kScreenHeight - 1
	    - static_cast<int>(GameConfig::kShockwaveRadiusCells) - 4;
	ASSERT_GT(far_y, 0);
	WorldTestAccess::AddEnemy(world, 50, far_y, 0);

	// 触发破盾
	WorldTestAccess::AddEnemy(world, 50, GameConfig::kScreenHeight - 1, 0);
	world.Tick(thunder::GameInput{}, kFrame);

	EXPECT_EQ(WorldTestAccess::EnemyCount(world), 1) << "半径外的敌机活着";
}

TEST(WorldShield, ShockwaveReachesFarEnemiesInsideRadius)
{
	GameWorld world = MakeQuietWorld();
	WorldTestAccess::AddPickup(world, 50, GameConfig::kScreenHeight - 1, PickupKind::Shield);
	world.Tick(thunder::GameInput{}, kFrame);

	// 12 格远：旧半径 10 打不到，现在的 14 应该扫得到
	const int far_y = GameConfig::kScreenHeight - 1 - 12;
	ASSERT_GT(far_y, 0);
	WorldTestAccess::AddEnemy(world, 50, far_y, 0);

	// 贴脸那只用来触发破盾
	WorldTestAccess::AddEnemy(world, 50, GameConfig::kScreenHeight - 1, 0);
	world.Tick(thunder::GameInput{}, kFrame);

	EXPECT_EQ(WorldTestAccess::EnemyCount(world), 0) << "半径内的远处敌机也要被扫掉";
}


// ========== 教程模式 ==========

TEST(WorldTutorial, KilledEnemiesNeverDropSupplies)
{
	// 教程里的补给全是脚本摆的：打死怪不该再随机掉东西出来
	GameWorld world = MakeQuietWorld();
	world.Reset(Difficulty::Easy, GameMode::Tutorial);
	WorldTestAccess::SetPlayer(world, 50, 40);

	int drops = 0;
	for(int i = 0; i < 150; ++i)
	{
		WorldTestAccess::ClearPickups(world);
		WorldTestAccess::TutorialSpawnEnemy(world, 50, 20, 0);
		WorldTestAccess::AddBullet(world, 51, 21);
		world.Tick(thunder::GameInput{}, kFrame);
		if(WorldTestAccess::PickupCount(world) > 0)
			++drops;
	}

	EXPECT_EQ(drops, 0) << "教程里击毁敌机不该掉落补给";
}

TEST(WorldTutorial, EnemyCollisionDoesNotHurtPlayer)
{
	GameWorld world = MakeQuietWorld();
	world.Reset(Difficulty::Easy, GameMode::Tutorial);
	WorldTestAccess::SetPlayer(world, 50, 40);

	const int life_before = world.Snapshot().life;
	WorldTestAccess::TutorialSpawnEnemy(world, 50, 40, 0); // 直接压在玩家身上
	world.Tick(thunder::GameInput{}, kFrame);

	EXPECT_EQ(world.Snapshot().life, life_before) << "教程里撞到不掉命";
}
TEST(WorldTutorial, FallenEnemiesDoNotCountAsKills)
{
	// 从底部漏下去的敌机不算"击毁"：教程的击落训练靠这个判定
	GameWorld world = MakeQuietWorld();
	world.Reset(Difficulty::Easy, GameMode::Tutorial);
	WorldTestAccess::SetPlayer(world, 50, 40);

	WorldTestAccess::TutorialSpawnEnemy(world, 20, 40, 3);
	WorldTestAccess::TutorialSpawnEnemy(world, 30, 40, 3);

	for(int i = 0; i < 400 && WorldTestAccess::EnemyCount(world) > 0; ++i)
		world.Tick(thunder::GameInput{}, kFrame);

	EXPECT_EQ(WorldTestAccess::EnemyCount(world), 0) << "应该都漏出屏幕了";
	EXPECT_EQ(world.Snapshot().defeated, 0) << "漏下去不能算击毁";
}

TEST(WorldTutorial, ShootingAnEnemyCountsAsKill)
{
	GameWorld world = MakeQuietWorld();
	world.Reset(Difficulty::Easy, GameMode::Tutorial);
	WorldTestAccess::SetPlayer(world, 50, 40);

	WorldTestAccess::TutorialSpawnEnemy(world, 50, 20, 0);
	WorldTestAccess::AddBullet(world, 51, 21);
	world.Tick(thunder::GameInput{}, kFrame);

	EXPECT_EQ(WorldTestAccess::EnemyCount(world), 0);
	EXPECT_EQ(world.Snapshot().defeated, 1);
}