#include <gtest/gtest.h>
#include "def.h"

//测试
TEST(ThunderFighterBasic, InitialState)
{
	ThunderFighter game(true);

	EXPECT_TRUE(game.IsRunning());
	EXPECT_EQ(game.GetState(), GameState::Menu);
	EXPECT_EQ(game.GetLife(), 5);
	EXPECT_EQ(game.GetScore(), 0);
}

//状态测试
TEST(ThunderFighterState, InitialStateIsValid)
{
	ThunderFighter game(true);

	GameState s = game.GetState();
	EXPECT_TRUE(s == GameState::Menu || s == GameState::Playing
	            || s == GameState::Exit);
}

//稳定性测试
TEST(ThunderFighterStability, MultipleConstructDestroy)
{
	for(int i = 0; i < 5; ++i)
	{
		ThunderFighter game(true);
		EXPECT_TRUE(game.IsRunning());
		EXPECT_EQ(game.GetState(), GameState::Menu);
	}
}

//分数测试
TEST(ThunderFighterLogic, ScoreStartsAtZero)
{
	ThunderFighter game(true);
	EXPECT_EQ(game.GetScore(), 0);
}

//生命测试
TEST(ThunderFighterLogic, LifeStartsAtFive)
{
	ThunderFighter game(true);
	EXPECT_EQ(game.GetLife(), 5);
}
