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

	// 作弊模式提示只在切换后短暂显示
	if(cheat_notice_ > steady_clock::duration::zero())
	{
		cheat_notice_ -= dt;
		if(cheat_notice_ < steady_clock::duration::zero())
			cheat_notice_ = steady_clock::duration::zero();
	}

	const bool input_ready = (cooldown_ == steady_clock::duration::zero());

	switch(screen_)
	{
	case Screen::Menu:
		// 作弊开关：主菜单按 F 切换（按下沿触发，长按不重复）。
		// 不受菜单冷却限制，任何时候都能切。状态默认不显示，
		// 只在切换后短暂提示一下。
		if(input_.Pressed(Key::CheatUnlock))
		{
			cheats_enabled_ = !cheats_enabled_;
			cheat_notice_ = ToDuration(milliseconds(GameConfig::kCheatNoticeMs));
		}

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
		// Q 与"游戏中返回主菜单"是同一个键，这里必须用按下沿：
		// 若用按住判定，玩家在游戏中按住 Q 回到主菜单后会立刻退出程序。
		else if(input_.Pressed(Key::Quit))
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
			// 直接同难度重开：结算界面上写的就是 "R RESTART"，
			// 回主菜单再选一次难度既慢又和界面承诺不符。
			StartGame(last_difficulty_);
		}
		else if(input_.Down(Key::Quit))
		{
			exit_requested_ = true;
		}
		break;
	}
}

void GameSession::StepPlaying(const InputState& input, bool input_ready, steady_clock::duration dt)
{
	if(input_ready && input.Pressed(Key::Quit))
	{
		screen_ = Screen::Menu;
		paused_ = false;
		cooldown_ = ToDuration(milliseconds(GameConfig::kAntiBounceDelayMs));
		return;
	}

	if(input_ready && input.Pressed(Key::Escape))
	{
		// 与 README 一致：游戏中 Esc 直接退出程序
		exit_requested_ = true;
		return;
	}

	// 暂停键：按下沿触发（长按只切换一次）。
	// 重构前的实现是电平触发，按住会以 200ms 为周期反复翻转。
	if(input_ready && input.Pressed(Key::Pause))
	{
		paused_ = !paused_;
	}

	// 暂停期间不推进世界（时间自然冻结）
	if(paused_)
		return;

	// 开局后的输入冷却期不把按键交给世界：
	// 选择难度的数字键（1/2/3）可能还没松开，而数字键在游戏中是作弊键，
	// 直接传入会导致"选完 Normal 立刻自杀"、"选完 Easy 立刻上帝模式"。
	GameInput game_input = input_ready ? GameInput::FromInput(input) : GameInput{};

	// 作弊模式未开启时屏蔽全部数字键，正常游玩不会误触
	if(!cheats_enabled_)
		game_input.digit.fill(false);

	world_.Tick(game_input, dt);

	if(world_.Over())
		Settle();
}

void GameSession::StartGame(Difficulty difficulty)
{
	last_difficulty_ = difficulty;
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

	final_score_ = snapshot.score + snapshot.bullets_remaining * GameConfig::kEndGameBulletBonus;

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
	snapshot.cheats_enabled = cheats_enabled_;
	snapshot.cheat_notice = cheat_notice_ > steady_clock::duration::zero();
	snapshot.difficulty = world_.difficulty();
	snapshot.world = world_.Snapshot();
	snapshot.leaderboard = highscore_.Scores();
	snapshot.rank = rank_;
	snapshot.new_record = new_record_;
	snapshot.entered_top5 = entered_top5_;
	snapshot.final_score = final_score_;
	return snapshot;
}

} // namespace thunder
