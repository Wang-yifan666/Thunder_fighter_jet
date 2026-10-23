// GameSession 流程测试：菜单跳转、暂停冻结、退出、结算与排行榜。
// 不接任何 UI，只用 RawInput + dt 驱动。

#include <gtest/gtest.h>

#include <chrono>

#include "game/game_config.h"
#include "game/game_session.h"
#include "game/input_state.h"
#include "game/score_store.h"
#include "game/session_snapshot.h"

using namespace std::chrono;
using thunder::GameSession;
using thunder::Key;
using thunder::MemoryScoreStore;
using thunder::RawInput;
using thunder::Screen;

namespace
{

RawInput Hold(Key key)
{
	RawInput raw;
	raw.down.set(thunder::KeyIndex(key));
	return raw;
}

void StepMs(GameSession& session, int ms, const RawInput& raw = RawInput{})
{
	session.Step(raw, duration_cast<steady_clock::duration>(milliseconds(ms)));
}

/// Menu -> 难度选择 -> Easy -> Playing，并清掉开局冷却。
void StartEasy(GameSession& session)
{
	StepMs(session, 16, Hold(Key::Digit1));
	StepMs(session, 250); // 清菜单冷却
	StepMs(session, 16, Hold(Key::Digit1));
	StepMs(session, 250); // 清开局冷却
}

} // namespace

TEST(Session, MenuNavigation)
{
	MemoryScoreStore store;
	GameSession session(store, 7);

	ASSERT_EQ(session.screen(), Screen::Menu);

	StepMs(session, 16, Hold(Key::Digit1));
	EXPECT_EQ(session.screen(), Screen::DifficultySelect);

	StepMs(session, 250);
	StepMs(session, 16, Hold(Key::Digit3));
	EXPECT_EQ(session.screen(), Screen::Playing);
	EXPECT_EQ(session.Snapshot().world.life, GameConfig::kHardLife);
	EXPECT_EQ(session.Snapshot().difficulty, Difficulty::Hard);
}

TEST(Session, DifficultyBackReturnsToMenu)
{
	MemoryScoreStore store;
	GameSession session(store, 7);

	StepMs(session, 16, Hold(Key::Digit1));
	ASSERT_EQ(session.screen(), Screen::DifficultySelect);

	StepMs(session, 250);
	StepMs(session, 16, Hold(Key::Back));
	EXPECT_EQ(session.screen(), Screen::Menu);
}

TEST(Session, LeaderboardNavigation)
{
	MemoryScoreStore store;
	GameSession session(store, 7);

	StepMs(session, 16, Hold(Key::Digit2));
	EXPECT_EQ(session.screen(), Screen::Leaderboard);

	StepMs(session, 250);
	StepMs(session, 16, Hold(Key::Back));
	EXPECT_EQ(session.screen(), Screen::Menu);
}

TEST(Session, ExitRequestedFromMenu)
{
	MemoryScoreStore store;
	GameSession session(store, 7);

	StepMs(session, 16, Hold(Key::Digit3));
	EXPECT_TRUE(session.exit_requested());
}

TEST(Session, PauseFreezesWorldTime)
{
	MemoryScoreStore store;
	GameSession session(store, 7);

	StartEasy(session);
	ASSERT_EQ(session.screen(), Screen::Playing);

	StepMs(session, 1000);
	EXPECT_EQ(session.Snapshot().world.elapsed_seconds, 1);

	StepMs(session, 16, Hold(Key::Pause));
	ASSERT_TRUE(session.Snapshot().paused);

	StepMs(session, 1000); // 暂停期间不得推进
	EXPECT_EQ(session.Snapshot().world.elapsed_seconds, 1);

	StepMs(session, 250); // 清暂停冷却
	StepMs(session, 16, Hold(Key::Pause));
	ASSERT_FALSE(session.Snapshot().paused);

	StepMs(session, 1000);
	EXPECT_EQ(session.Snapshot().world.elapsed_seconds, 2);
}

TEST(Session, PauseTogglesOnlyOnceWhileHeld)
{
	MemoryScoreStore store;
	GameSession session(store, 7);

	StartEasy(session);
	ASSERT_EQ(session.screen(), Screen::Playing);

	StepMs(session, 16, Hold(Key::Pause));
	ASSERT_TRUE(session.Snapshot().paused);

	// 持续按住不应反复翻转（重构前会以 200ms 为周期抖动）
	for(int i = 0; i < 20; ++i)
		StepMs(session, 16, Hold(Key::Pause));
	EXPECT_TRUE(session.Snapshot().paused);

	// 松开后再按才切换
	StepMs(session, 16);
	StepMs(session, 16, Hold(Key::Pause));
	EXPECT_FALSE(session.Snapshot().paused);
}

TEST(Session, QuitReturnsToMenu)
{
	MemoryScoreStore store;
	GameSession session(store, 7);

	StartEasy(session);
	ASSERT_EQ(session.screen(), Screen::Playing);

	StepMs(session, 16, Hold(Key::Quit));
	EXPECT_EQ(session.screen(), Screen::Menu);
	EXPECT_FALSE(session.Snapshot().paused);
}

// Esc 在游戏中直接退出程序（与 README 一致）
TEST(Session, EscapeRequestsExit)
{
	MemoryScoreStore store;
	GameSession session(store, 7);

	StartEasy(session);
	ASSERT_EQ(session.screen(), Screen::Playing);

	StepMs(session, 16, Hold(Key::Escape));
	EXPECT_TRUE(session.exit_requested());
}

TEST(Session, EscapeWorksWhilePaused)
{
	MemoryScoreStore store;
	GameSession session(store, 7);

	StartEasy(session);
	StepMs(session, 16, Hold(Key::Pause));
	ASSERT_TRUE(session.Snapshot().paused);

	StepMs(session, 16, Hold(Key::Escape));
	EXPECT_TRUE(session.exit_requested());
}

TEST(Session, GameOverSubmitsScoreAndRestarts)
{
	MemoryScoreStore store;
	GameSession session(store, 7);

	StartEasy(session);
	ASSERT_EQ(session.screen(), Screen::Playing);

	const auto before = session.Snapshot().world;
	const int expected = before.score
	    + before.bullets_remaining * GameConfig::kEndGameBulletBonus;

	StepMs(session, 16, Hold(Key::Digit2)); // 作弊自杀

	const auto snap = session.Snapshot();
	ASSERT_EQ(snap.screen, Screen::GameOver);
	EXPECT_EQ(snap.world.life, 0);
	ASSERT_FALSE(snap.leaderboard.empty());
	EXPECT_EQ(snap.leaderboard.front(), expected);
	EXPECT_EQ(snap.final_score, expected);
	EXPECT_EQ(snap.rank, 1);
	EXPECT_TRUE(snap.new_record);
	EXPECT_TRUE(snap.entered_top5);

	// 已写入存储
	const auto persisted = store.Load();
	ASSERT_FALSE(persisted.empty());
	EXPECT_EQ(persisted.front(), expected);

	StepMs(session, 16, Hold(Key::Restart));
	EXPECT_EQ(session.screen(), Screen::Menu);
}

TEST(Session, ExitFromGameOverSetsExitRequested)
{
	MemoryScoreStore store;
	GameSession session(store, 7);

	StartEasy(session);
	StepMs(session, 16, Hold(Key::Digit2));
	ASSERT_EQ(session.screen(), Screen::GameOver);

	StepMs(session, 16, Hold(Key::Quit));
	EXPECT_TRUE(session.exit_requested());
}
