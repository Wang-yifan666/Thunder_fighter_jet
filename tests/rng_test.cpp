// 随机数归属测试：RNG 由 GameWorld 持有，seed 相同则场景可复现。

#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <vector>

#include "game/game_types.h"
#include "game/game_world.h"
#include "game/world_snapshot.h"

using namespace std::chrono;
using thunder::EntityView;
using thunder::GameInput;
using thunder::GameWorld;

namespace
{

/// 跑一段包含刷怪与横向移动（4 级起）的场景。
std::vector<EntityView> RunScene(std::uint32_t seed)
{
	GameWorld world(seed);
	world.Reset(Difficulty::Hard);

	const GameInput none;
	for(int i = 0; i < 30; ++i) // 累计 30 秒 -> 4 级，横向移动生效
		world.Tick(none, seconds(1));

	for(int i = 0; i < 120; ++i)
		world.Tick(none, milliseconds(16));

	return world.Snapshot().enemies;
}

bool SameScene(const std::vector<EntityView>& a,
               const std::vector<EntityView>& b)
{
	if(a.size() != b.size())
		return false;

	for(size_t i = 0; i < a.size(); ++i)
	{
		if(a[i].x != b[i].x || a[i].y != b[i].y)
			return false;
	}
	return true;
}

} // namespace

TEST(Rng, SameSeedReplaysSameScene)
{
	const auto first = RunScene(12345);
	const auto second = RunScene(12345);

	ASSERT_FALSE(first.empty());
	EXPECT_TRUE(SameScene(first, second));
}

TEST(Rng, DifferentSeedsProduceDifferentScenes)
{
	const auto a = RunScene(1);
	const auto b = RunScene(2);

	ASSERT_FALSE(a.empty());
	ASSERT_FALSE(b.empty());
	EXPECT_FALSE(SameScene(a, b));
}
