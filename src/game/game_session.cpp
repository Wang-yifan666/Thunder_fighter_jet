// GameSession：把重构前终端里的"阻塞式菜单循环"改写为非阻塞状态机。
// 按键语义、冷却时长、结算与排行榜规则与终端现状保持一致；
// 唯一的结构差异是：不再 Sleep，改为 cooldown 计时。

#include "game/game_session.h"

using namespace std::chrono;

namespace thunder
{
namespace
{

steady_clock::duration ToDuration(milliseconds ms)
{
	return duration_cast<steady_clock::duration>(ms);
}

constexpr int kPauseCooldownMs = 200; // 对应重构前 DrawFrame 里 200ms 的等待

} // namespace

GameSession::GameSession(ScoreStore& store, std::uint32_t rng_seed):
world_(rng_seed), highscore_(store)
{
	highscore_.Load();
}

void GameSession::Step(const RawInput& raw, steady_clock::duration dt)
{
	input_ = tracker_.Update(raw);

	if(cooldown_ > steady_clock::duration::zero())
	{
		cooldown_ -= dt;
		if(cooldown_ < steady_clock::duration::zero())
			cooldown_ = steady_clock::duration::zero();
	}

	const bool input_ready = (cooldown_ == steady_clock::duration::zero());

	switch(screen_)
	{
	case Screen::Menu:
		if(!input_ready)
			break;

		if(input_.Down(Key::Digit1))
		{
			cooldown_ = ToDuration(milliseconds(GameConfig::kKeyDebounceMs));
			screen_ = Screen::DifficultySelect;
		}
		else if(input_.Down(Key::Digit2))
		{
			cooldown_ = ToDuration(milliseconds(GameConfig::kKeyDebounceMs));
			highscore_.Load();
			screen_ = Screen::Leaderboard;
		}
		else if(input_.Down(Key::Digit3))
		{
			exit_requested_ = true;
		}
		break;

	case Screen::DifficultySelect:
		if(!input_ready)
			break;

		if(input_.Down(Key::Digit1))
			StartGame(Difficulty::Easy);
		else if(input_.Down(Key::Digit2))
			StartGame(Difficulty::Normal);
		else if(input_.Down(Key::Digit3))
			StartGame(Difficulty::Hard);
		else if(input_.Down(Key::Back))
		{
			cooldown_ = ToDuration(milliseconds(GameConfig::kKeyDebounceMs));
			screen_ = Screen::Menu;
		}
		break;

	case Screen::Leaderboard:
		if(!input_ready)
			break;

		if(input_.Down(Key::Back))
		{
			cooldown_ = ToDuration(milliseconds(GameConfig::kKeyDebounceMs));
			screen_ = Screen::Menu;
		}
		else if(input_.Down(Key::Escape))
		{
			exit_requested_ = true;
		}
		break;

	case Screen::Playing:
		StepPlaying(input_, input_ready, dt);
		break;

	case Screen::GameOver:
		if(input_.Down(Key::Restart))
		{
			screen_ = Screen::Menu;
		}
		else if(input_.Down(Key::Quit))
		{
			exit_requested_ = true;
		}
		break;
	}
}

void GameSession::StepPlaying(const InputState& input, bool input_ready,
                              steady_clock::duration dt)
{
	if(input_ready && input.Pressed(Key::Quit))
	{
		screen_ = Screen::Menu;
		paused_ = false;
		cooldown_ = ToDuration(milliseconds(GameConfig::kAntiBounceDelayMs));
		return;
	}

	// 暂停键沿用重构前的电平触发 + 200ms 冷却（B6 单独改为按下沿触发）
	if(input_ready && input.Down(Key::Pause))
	{
		paused_ = !paused_;
		cooldown_ = ToDuration(milliseconds(kPauseCooldownMs));
	}

	// 暂停期间不推进世界（时间自然冻结）
	if(paused_)
		return;

	// 注意：当前版本 Esc 在游戏中不生效，与重构前一致；B7 单独修复。

	world_.Tick(GameInput::FromInput(input), dt);

	if(world_.Over())
		Settle();
}

void GameSession::StartGame(Difficulty difficulty)
{
	world_.Reset(difficulty);

	paused_ = false;
	rank_ = 0;
	new_record_ = false;
	entered_top5_ = false;
	final_score_ = 0;

	screen_ = Screen::Playing;
	cooldown_ = ToDuration(milliseconds(GameConfig::kKeyDebounceMs));
}

void GameSession::Settle()
{
	const WorldSnapshot snapshot = world_.Snapshot();

	final_score_ = snapshot.score
	    + snapshot.bullets_remaining * GameConfig::kEndGameBulletBonus;

	highscore_.Submit(final_score_);
	rank_ = highscore_.GetRank(final_score_);
	new_record_ = (rank_ == 1);
	entered_top5_ = (rank_ <= GameConfig::kMaxHighScores);

	screen_ = Screen::GameOver;
}

SessionSnapshot GameSession::Snapshot() const
{
	SessionSnapshot snapshot;
	snapshot.screen = screen_;
	snapshot.paused = paused_;
	snapshot.exit_requested = exit_requested_;
	snapshot.difficulty = world_.difficulty();
	snapshot.world = world_.Snapshot();
	snapshot.leaderboard = highscore_.Scores();
	snapshot.rank = rank_;
	snapshot.new_record = new_record_;
	snapshot.entered_top5 = entered_top5_;
	return snapshot;
}

} // namespace thunder
