#pragma once

#include <chrono>
#include <cstdint>

#include "game/game_config.h"
#include "game/game_world.h"
#include "game/highscore.h"
#include "game/input_state.h"
#include "game/score_store.h"
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

	void Step(const RawInput& raw, std::chrono::steady_clock::duration dt);

	SessionSnapshot Snapshot() const;

	bool exit_requested() const { return exit_requested_; }
	Screen screen() const { return screen_; }

private:
	void StartGame(Difficulty difficulty);
	void StepPlaying(const InputState& input, bool input_ready,
	                 std::chrono::steady_clock::duration dt);
	void Settle();

	GameWorld world_;
	HighScore highscore_;
	InputTracker tracker_;
	InputState input_;

	Screen screen_ = Screen::Menu;
	bool paused_ = false;
	bool exit_requested_ = false;
	std::chrono::steady_clock::duration cooldown_{};

	int rank_ = 0;
	bool new_record_ = false;
	bool entered_top5_ = false;
	int final_score_ = 0;
};

} // namespace thunder
