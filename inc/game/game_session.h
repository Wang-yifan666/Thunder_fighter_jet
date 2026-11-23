#pragma once

#include <chrono>
#include <cstdint>

#include "game/game_config.h"
#include "game/game_world.h"
#include "game/highscore.h"
#include "game/input_state.h"
#include "game/score_store.h"
#include "game/settings_store.h"
#include "game/session_snapshot.h"

namespace thunder
{

/// 非阻塞流程状态机：Menu → DifficultySelect → Playing → GameOver → Menu。
/// - 不阻塞、不睡眠、不读系统时钟（时间通过 dt 注入）
/// - 暂停时绝不调用 GameWorld::Tick
/// - 独占自己的 InputTracker（每进程至少一个前端只持有一个会话）
class GameSession
{
public:
	explicit GameSession(ScoreStore& store, std::uint32_t rng_seed = 0);

	/// 带设置的构造：正式游戏传入 FileSettingsStore，测试用内存实现。
	GameSession(ScoreStore& store, SettingsStore& settings, std::uint32_t rng_seed = 0);

	void Step(const RawInput& raw, std::chrono::steady_clock::duration dt);

	SessionSnapshot Snapshot() const;

	bool exit_requested() const
	{
		return exit_requested_;
	}
	Screen screen() const
	{
		return screen_;
	}

private:
	void StartGame(Difficulty difficulty);
	void StepPlaying(const InputState& input, bool input_ready,
	                 std::chrono::steady_clock::duration dt);
	void Settle();

	// ========== 新手教程 ==========
	void StartTutorial();
	void StepTutorial(const InputState& input, bool input_ready,
	                  std::chrono::steady_clock::duration dt);
	/// 当前步骤要摆的东西（敌机 / 补给）在进入这一步时一次性摆好
	void ArmTutorialStep();
	void SetTutorialStep(TutorialStep step);
	/// 达成目标且停顿够了之后：进入下一步，最后一步则收尾
	void AdvanceTutorial();
	void FinishTutorial(bool solved);

	GameWorld world_;
	HighScore highscore_;
	InputTracker tracker_;
	InputState input_;

	/// 没从外部注入设置存储时用的内存实现（测试默认走这里）
	MemorySettingsStore owned_settings_;
	SettingsStore* settings_ = &owned_settings_;
	PlayerSettings player_settings_;

	Screen screen_ = Screen::Menu;
	/// 上一局选的难度：结算界面按 R 直接同难度再来一局
	Difficulty last_difficulty_ = Difficulty::Easy;
	bool paused_ = false;
	bool exit_requested_ = false;
	/// 主菜单按 F 切换；关闭时游戏内的数字键作弊被屏蔽
	bool cheats_enabled_ = false;
	/// 切换作弊模式后的提示剩余时长（状态本身不常驻显示）
	std::chrono::steady_clock::duration cheat_notice_{};
	std::chrono::steady_clock::duration cooldown_{};

	int rank_ = 0;
	bool new_record_ = false;
	bool entered_top5_ = false;
	int final_score_ = 0;

	// 教程进度
	TutorialStep tutorial_step_ = TutorialStep::Move;
	bool tutorial_solved_ = false;
	/// 本次是否"刚刚"解锁（第一次通关才算，第二次不该再喊解锁）
	bool tutorial_unlocked_now_ = false;
	/// 当前步骤的目标物是否已经摆好
	bool tutorial_armed_ = false;
	/// 当前步骤已经停留了多久 / 达成目标后又停了多久
	std::chrono::steady_clock::duration tutorial_step_time_{};
	std::chrono::steady_clock::duration tutorial_hold_time_{};
	/// 当前步骤的目标是否已经达成（达成后先显示一下"做得好"再切）
	bool tutorial_step_done_ = false;
	/// 移动训练：累计挪动格数（来回走也算，不能靠原地抖）
	int tutorial_moved_cells_ = 0;
	int tutorial_last_x_ = 0;
	int tutorial_last_y_ = 0;
	int tutorial_baseline_x_ = 0;
	int tutorial_baseline_y_ = 0;
	int tutorial_baseline_ammo_ = 0;
	/// 拾取训练：四种补给各自有没有拿到
	bool tutorial_got_health_ = false;
	bool tutorial_got_ammo_ = false;
	bool tutorial_got_shield_ = false;
	bool tutorial_got_firepower_ = false;
	int tutorial_prev_life_ = 0;
	int tutorial_prev_ammo_ = 0;
};

} // namespace thunder
