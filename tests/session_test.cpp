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

/// Menu -> 难度选择 -> Hard -> Playing，并清掉开局冷却。
void StartHard(GameSession& session)
{
	StepMs(session, 16, Hold(Key::Digit1));
	StepMs(session, 250); // 清菜单冷却
	StepMs(session, 16, Hold(Key::Digit3));
	StepMs(session, 250); // 清开局冷却
}

/// 在主菜单按一次 F 打开作弊模式（游戏内数字键才会生效）。
void EnableCheats(GameSession& session)
{
	StepMs(session, 16, Hold(Key::CheatUnlock));
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

// 难度选择键不能泄漏进游戏：选中 Normal 后按键仍被按住时，
// 数字键 2 是作弊"自杀"，必须被起始冷却挡住。
TEST(Session, DifficultyKeyDoesNotLeakIntoGame)
{
	MemoryScoreStore store;
	GameSession session(store, 7);

	StepMs(session, 16, Hold(Key::Digit1)); // 菜单 -> 难度
	StepMs(session, 250);                   // 清菜单冷却

	StepMs(session, 16, Hold(Key::Digit2)); // 选择 Normal，手指还没松开
	ASSERT_EQ(session.screen(), Screen::Playing);
	ASSERT_EQ(session.Snapshot().difficulty, Difficulty::Normal);

	for(int i = 0; i < 6; ++i) // 96ms，仍在起始冷却内
		StepMs(session, 16, Hold(Key::Digit2));

	EXPECT_EQ(session.screen(), Screen::Playing);
	EXPECT_EQ(session.Snapshot().world.life, GameConfig::kNormalLife);
}

// 同理：Easy 的"1"是上帝模式，也不能在开局瞬间触发。
TEST(Session, EasyCheatKeyDoesNotLeakIntoGame)
{
	MemoryScoreStore store;
	GameSession session(store, 7);

	StepMs(session, 16, Hold(Key::Digit1));
	StepMs(session, 250);
	StepMs(session, 16, Hold(Key::Digit1)); // 选择 Easy，按键仍按住
	ASSERT_EQ(session.screen(), Screen::Playing);

	for(int i = 0; i < 6; ++i)
		StepMs(session, 16, Hold(Key::Digit1));

	EXPECT_EQ(session.Snapshot().world.life, GameConfig::kEasyLife);
	EXPECT_EQ(session.Snapshot().world.bullets_remaining, GameConfig::kEasyBullets);
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

	// 主菜单退出用 Q（原为数字键 3）
	StepMs(session, 16, Hold(Key::Quit));
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

	EnableCheats(session);
	StartEasy(session);
	ASSERT_EQ(session.screen(), Screen::Playing);

	const auto before = session.Snapshot().world;
	const int expected = before.score + before.bullets_remaining * GameConfig::kEndGameBulletBonus;

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

	// R 直接同难度重开（结算界面写的就是 "R RESTART"）
	StepMs(session, 16, Hold(Key::Restart));
	EXPECT_EQ(session.screen(), Screen::Playing);
	EXPECT_EQ(session.Snapshot().difficulty, Difficulty::Easy);
	EXPECT_GT(session.Snapshot().world.life, 0) << "新一局是满血";
	EXPECT_EQ(session.Snapshot().world.level, 1);
}

TEST(Session, RestartKeepsLastDifficulty)
{
	MemoryScoreStore store;
	GameSession session(store, 7);

	EnableCheats(session);
	StartHard(session);
	ASSERT_EQ(session.screen(), Screen::Playing);
	ASSERT_EQ(session.Snapshot().difficulty, Difficulty::Hard);

	StepMs(session, 16, Hold(Key::Digit2)); // 作弊自杀
	ASSERT_EQ(session.screen(), Screen::GameOver);

	StepMs(session, 16, Hold(Key::Restart));
	EXPECT_EQ(session.screen(), Screen::Playing);
	EXPECT_EQ(session.Snapshot().difficulty, Difficulty::Hard) << "重开要保持上次的难度";
}

TEST(Session, ExitFromGameOverSetsExitRequested)
{
	MemoryScoreStore store;
	GameSession session(store, 7);

	EnableCheats(session);
	StartEasy(session);
	StepMs(session, 16, Hold(Key::Digit2));
	ASSERT_EQ(session.screen(), Screen::GameOver);

	StepMs(session, 16, Hold(Key::Quit));
	EXPECT_TRUE(session.exit_requested());
}

// ===================================================================
// 作弊开关：默认关闭，只有主菜单按 F 才在游戏内生效
// ===================================================================

TEST(Session, CheatsDisabledByDefault)
{
	MemoryScoreStore store;
	GameSession session(store, 7);

	EXPECT_FALSE(session.Snapshot().cheats_enabled);

	StartEasy(session);
	ASSERT_EQ(session.screen(), Screen::Playing);
	const int life_before = session.Snapshot().world.life;

	// 数字键 2 是"自杀"，作弊关闭时必须完全无效
	for(int i = 0; i < 10; ++i)
		StepMs(session, 16, Hold(Key::Digit2));

	EXPECT_EQ(session.screen(), Screen::Playing);
	EXPECT_EQ(session.Snapshot().world.life, life_before);
}

TEST(Session, CheatsEnabledAfterMenuToggle)
{
	MemoryScoreStore store;
	GameSession session(store, 7);

	EnableCheats(session);
	EXPECT_TRUE(session.Snapshot().cheats_enabled);

	StartEasy(session);
	ASSERT_EQ(session.screen(), Screen::Playing);

	StepMs(session, 16, Hold(Key::Digit2)); // 作弊自杀，此时应生效
	EXPECT_EQ(session.screen(), Screen::GameOver);
}

TEST(Session, CheatToggleSwitchesBackOff)
{
	MemoryScoreStore store;
	GameSession session(store, 7);

	EnableCheats(session);
	EXPECT_TRUE(session.Snapshot().cheats_enabled);

	// 松开后再按一次 F，应关闭
	StepMs(session, 16);
	EnableCheats(session);
	EXPECT_FALSE(session.Snapshot().cheats_enabled);
}

// 作弊状态不常驻显示：只在切换后短暂提示，随后自动消失
TEST(Session, CheatNoticeIsTransient)
{
	MemoryScoreStore store;
	GameSession session(store, 7);

	EXPECT_FALSE(session.Snapshot().cheat_notice);

	EnableCheats(session);
	EXPECT_TRUE(session.Snapshot().cheat_notice);
	EXPECT_TRUE(session.Snapshot().cheats_enabled);

	// 提示时长内仍然显示
	StepMs(session, 200);
	EXPECT_TRUE(session.Snapshot().cheat_notice);

	// 超过 kCheatNoticeMs 后自动消失，但作弊仍然是开启的
	StepMs(session, GameConfig::kCheatNoticeMs);
	EXPECT_FALSE(session.Snapshot().cheat_notice);
	EXPECT_TRUE(session.Snapshot().cheats_enabled);
}

// Q 同时是"游戏中回主菜单"和"主菜单退出"，按住 Q 回菜单后不能顺势退出程序
TEST(Session, HoldingQuitDoesNotExitFromMenu)
{
	MemoryScoreStore store;
	GameSession session(store, 7);

	StartEasy(session);
	ASSERT_EQ(session.screen(), Screen::Playing);

	StepMs(session, 16, Hold(Key::Quit)); // 游戏内按 Q 回主菜单
	ASSERT_EQ(session.screen(), Screen::Menu);

	for(int i = 0; i < 20; ++i) // 一直按着 Q 不放
		StepMs(session, 16, Hold(Key::Quit));

	EXPECT_EQ(session.screen(), Screen::Menu);
	EXPECT_FALSE(session.exit_requested());
}

// ========== 新手教程 ==========

TEST(Session, TutorialStartsFromMenu)
{
	MemoryScoreStore store;
	GameSession session(store, 7);

	ASSERT_EQ(session.screen(), Screen::Menu);
	StepMs(session, 16, Hold(Key::Digit3));

	EXPECT_EQ(session.screen(), Screen::Tutorial);
	EXPECT_EQ(session.Snapshot().tutorial_step, TutorialStep::Move);
}

TEST(Session, TutorialSkipLandsOnDoneWithoutUnlock)
{
	MemoryScoreStore store;
	GameSession session(store, 7);
	StepMs(session, 16, Hold(Key::Digit3));
	ASSERT_EQ(session.screen(), Screen::Tutorial);

	StepMs(session, 250); // 等开局冷却
	StepMs(session, 16, Hold(Key::Quit));
	EXPECT_EQ(session.screen(), Screen::TutorialDone);
	EXPECT_FALSE(session.Snapshot().tutorial_solved) << "跳过不算通关";
	EXPECT_FALSE(session.Snapshot().tutorial_done) << "跳过不该解锁装扮";
}
TEST(Session, TutorialStepStaysForAWhileBeforeAdvancing)
{
	MemoryScoreStore store;
	GameSession session(store, 7);
	StepMs(session, 16, Hold(Key::Digit3));
	ASSERT_EQ(session.screen(), Screen::Tutorial);
	StepMs(session, 250); // 清开局冷却
	ASSERT_EQ(session.Snapshot().tutorial_step, TutorialStep::Move);

	// 一路按着左键走满 1 秒：移动量早够了，但提示必须至少停留 kTutorialStepMinMs
	for(int i = 0; i < 60; ++i)
		StepMs(session, 16, Hold(Key::Left));
	EXPECT_EQ(session.Snapshot().tutorial_step, TutorialStep::Move)
	    << "提示还没停留够，不该切步骤";

	// 再走 2 秒（累计超过最短停留 + 完成停顿）才进入下一步
	for(int i = 0; i < 120; ++i)
		StepMs(session, 16, Hold(Key::Left));
	EXPECT_EQ(session.Snapshot().tutorial_step, TutorialStep::Fire);
}

TEST(Session, TutorialWobbleStillCountsAsMovement)
{
	MemoryScoreStore store;
	GameSession session(store, 7);
	StepMs(session, 16, Hold(Key::Digit3));
	StepMs(session, 250);

	// 左右来回走也算移动：不能因为"回到起点"就判定没动过
	for(int i = 0; i < 40; ++i)
		StepMs(session, 16, Hold(Key::Left));
	for(int i = 0; i < 40; ++i)
		StepMs(session, 16, Hold(Key::Right));
	for(int i = 0; i < 120; ++i)
		StepMs(session, 16);

	EXPECT_EQ(session.Snapshot().tutorial_step, TutorialStep::Fire);
}
TEST(Session, TutorialRestartsFromTheBeginning)
{
	MemoryScoreStore store;
	thunder::MemorySettingsStore settings;
	GameSession session(store, settings, 7);

	// 第一次进教程，走一段把移动训练完成
	StepMs(session, 16, Hold(Key::Digit3));
	StepMs(session, 250);
	for(int i = 0; i < 200; ++i)
		StepMs(session, 16, Hold(Key::Left));
	ASSERT_EQ(session.Snapshot().tutorial_step, TutorialStep::Fire);

	// 跳过 -> 回主菜单
	StepMs(session, 16, Hold(Key::Quit));
	ASSERT_EQ(session.screen(), Screen::TutorialDone);
	StepMs(session, 250);
	StepMs(session, 16, Hold(Key::Digit2));
	ASSERT_EQ(session.screen(), Screen::Menu);
	StepMs(session, 250);

	// 再进一次：必须从"移动"这一步重新开始
	StepMs(session, 16, Hold(Key::Digit3));
	ASSERT_EQ(session.screen(), Screen::Tutorial);
	EXPECT_EQ(session.Snapshot().tutorial_step, TutorialStep::Move);

	// 站着不动等过最短停留：仍然要停在移动训练（累计移动距离被清零了）
	for(int i = 0; i < 160; ++i)
		StepMs(session, 16);
	EXPECT_EQ(session.Snapshot().tutorial_step, TutorialStep::Move)
	    << "第二次进入教程不该直接跳过移动训练";
}

TEST(Session, TutorialCompletionOnlyAnnouncesUnlockOnce)
{
	MemoryScoreStore store;
	thunder::MemorySettingsStore settings;

	thunder::PlayerSettings already_done;
	already_done.tutorial_done = true;
	settings.Save(already_done);

	GameSession session(store, settings, 7);
	StepMs(session, 16, Hold(Key::Digit3));
	StepMs(session, 250);
	StepMs(session, 16, Hold(Key::Quit));

	EXPECT_EQ(session.screen(), Screen::TutorialDone);
	EXPECT_FALSE(session.Snapshot().tutorial_newly_unlocked)
	    << "已经解锁过，第二次通关不该再喊一次解锁";
}
TEST(Session, TutorialFullPlaythroughUnlocksSkin)
{
	MemoryScoreStore store;
	thunder::MemorySettingsStore settings;
	GameSession session(store, settings, 7);

	StepMs(session, 16, Hold(Key::Digit3));
	StepMs(session, 250);

	// 移动训练：往左走一段
	for(int i = 0; i < 100; ++i)
		StepMs(session, 16, Hold(Key::Left));

	// 射击/击落：按住开火（补给训练期间会自动禁火）
	for(int i = 0; i < 1200 && session.screen() == Screen::Tutorial; ++i)
		StepMs(session, 16, Hold(Key::Fire));

	// 补给会落在玩家所在列，站着等它们掉下来
	for(int i = 0; i < 1200 && session.screen() == Screen::Tutorial; ++i)
		StepMs(session, 16);

	ASSERT_EQ(session.screen(), Screen::TutorialDone)
	    << "完整走完教程应该进结束页，当前步骤 "
	    << static_cast<int>(session.Snapshot().tutorial_step);
	EXPECT_TRUE(session.Snapshot().tutorial_solved);
	EXPECT_TRUE(session.Snapshot().tutorial_newly_unlocked) << "第一次通关要提示解锁";
	EXPECT_TRUE(settings.Load().tutorial_done) << "进度要落盘";
}
TEST(Session, TutorialAmmoPackIsDetectedWhileFiring)
{
	// 拾取训练不再禁火：玩家一边开火一边捡弹药包，也要能判定"捡到了"
	MemoryScoreStore store;
	thunder::MemorySettingsStore settings;
	GameSession session(store, settings, 7);

	StepMs(session, 16, Hold(Key::Digit3));
	StepMs(session, 250);
	for(int i = 0; i < 100; ++i)
		StepMs(session, 16, Hold(Key::Left));

	// 一路按住开火，直到走完开火与击落两步
	for(int i = 0; i < 1500 && session.Snapshot().tutorial_step != TutorialStep::PickupAmmo; ++i)
		StepMs(session, 16, Hold(Key::Fire));

	ASSERT_EQ(session.Snapshot().tutorial_step, TutorialStep::PickupAmmo);

	// 继续按着开火，等弹药包落到身上：不开火测试里会因为弹药被消耗而漏判
	for(int i = 0; i < 900 && session.Snapshot().tutorial_step == TutorialStep::PickupAmmo; ++i)
		StepMs(session, 16, Hold(Key::Fire));

	EXPECT_NE(session.Snapshot().tutorial_step, TutorialStep::PickupAmmo)
	    << "边开火边捡弹药包也要能过关";
}