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
	player_settings_ = settings_->Load();
}

GameSession::GameSession(ScoreStore& store, SettingsStore& settings, std::uint32_t rng_seed):
world_(rng_seed), highscore_(store), settings_(&settings)
{
	highscore_.Load();
	player_settings_ = settings_->Load();
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
		else if(input_.Down(Key::Digit3))
		{
			StartTutorial();
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

	case Screen::Tutorial:
		StepTutorial(input_, input_ready, dt);
		break;

	case Screen::TutorialDone:
		if(!input_ready)
			break;

		if(input_.Down(Key::Digit1))
		{
			cooldown_ = ToDuration(milliseconds(GameConfig::kKeyDebounceMs));
			screen_ = Screen::DifficultySelect;
		}
		else if(input_.Down(Key::Digit2) || input_.Pressed(Key::Quit))
		{
			cooldown_ = ToDuration(milliseconds(GameConfig::kKeyDebounceMs));
			screen_ = Screen::Menu;
		}
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

// ========== 新手教程 ==========
// 编排放在会话层，GameWorld 只提供"摆一只/摆一堆"的入口。
// 每一步都由快照里的事实推进（动过没有 / 开过枪没有 / 敌机还在不在 / 补给拿到没有），
// 不看计时——否则玩家发呆就会卡住。

void GameSession::StartTutorial()
{
	tutorial_step_ = TutorialStep::Move;
	tutorial_solved_ = false;
	tutorial_armed_ = false;

	world_.Reset(Difficulty::Easy, GameMode::Tutorial);
	// 放在中下方，四周留出空间方便演示移动与拾取
	world_.TutorialSetPlayer(GameConfig::kScreenWidth / 2 - 1, GameConfig::kScreenHeight - 7);

	const WorldSnapshot snap = world_.Snapshot();
	tutorial_baseline_x_ = snap.player_x;
	tutorial_baseline_y_ = snap.player_y;
	tutorial_baseline_ammo_ = snap.bullets_remaining;

	screen_ = Screen::Tutorial;
	cooldown_ = ToDuration(milliseconds(GameConfig::kKeyDebounceMs));
}

void GameSession::SetTutorialStep(TutorialStep step)
{
	tutorial_step_ = step;
	tutorial_armed_ = false; // 下一步的目标物在下一帧摆

	// 刚进入拾取训练：四种补给都还没拿到
	if(step == TutorialStep::Pickup)
	{
		tutorial_got_health_ = false;
		tutorial_got_ammo_ = false;
		tutorial_got_shield_ = false;
		tutorial_got_firepower_ = false;
	}
}

void GameSession::ArmTutorialStep()
{
	const WorldSnapshot snap = world_.Snapshot();

	switch(tutorial_step_)
	{
	case TutorialStep::Kill:
		// 慢速直线小怪：speed 1（每 20 帧下移一格），等级 1 不会左右乱走。
		// 对齐玩家当前列，保证站着不动也能打中。
		world_.TutorialSpawnEnemy(snap.player_x - 1, 6, 1);
		tutorial_baseline_ammo_ = snap.bullets_remaining;
		break;

	case TutorialStep::Pickup:
	{
		// 四种补给横着摆一排，从玩家上方落下。
		// 重新摆的时候只补"还没拿到的"，不能把已经拿到的进度清零。
		const PickupKind kinds[4] = {PickupKind::Health, PickupKind::Ammo, PickupKind::Shield,
		                             PickupKind::Firepower};
		const bool got[4] = {tutorial_got_health_, tutorial_got_ammo_, tutorial_got_shield_,
		                     tutorial_got_firepower_};
		// 摆在玩家正上方、一格一格的纵向队列里：新手站着不动也能顺序捡到，
		// 不用满屏找补给。
		const int x = std::max(1, snap.player_x);
		const int top = std::max(1, snap.player_y - 13);

		int slot = 0;
		for(int i = 0; i < 4; ++i)
		{
			if(got[i])
				continue;
			world_.TutorialSpawnPickup(x, top + slot * 4, kinds[i]);
			++slot;
		}

		tutorial_prev_life_ = snap.life;
		tutorial_prev_ammo_ = snap.bullets_remaining;
		break;
	}

	default:
		break;
	}

	tutorial_armed_ = true;
}

void GameSession::StepTutorial(const InputState& input, bool input_ready, steady_clock::duration dt)
{
	// Q / Esc 随时跳过：跳到结束页，但不算通关（不解锁装扮）
	if(input_ready && (input.Pressed(Key::Quit) || input.Pressed(Key::Escape)))
	{
		FinishTutorial(false);
		return;
	}

	GameInput game_input = input_ready ? GameInput::FromInput(input) : GameInput{};
	// 教程里禁用作弊键：数字键在教程中是别的用途，也不该把教程打乱
	game_input.digit.fill(false);
	// 拾取训练时禁火：否则弹药包刚补的 50 发会被自己打掉，没法判断"捡到了没有"
	if(tutorial_step_ == TutorialStep::Pickup)
		game_input.fire = false;

	world_.Tick(game_input, dt);

	if(!tutorial_armed_)
		ArmTutorialStep();

	const WorldSnapshot snap = world_.Snapshot();

	switch(tutorial_step_)
	{
	case TutorialStep::Move:
		if(snap.player_x != tutorial_baseline_x_ || snap.player_y != tutorial_baseline_y_)
			SetTutorialStep(TutorialStep::Fire);
		break;

	case TutorialStep::Fire:
		if(snap.bullets_remaining < tutorial_baseline_ammo_)
			SetTutorialStep(TutorialStep::Kill);
		break;

	case TutorialStep::Kill:
		if(snap.enemies.empty())
			SetTutorialStep(TutorialStep::Pickup);
		break;

	case TutorialStep::Pickup:
		if(snap.life > tutorial_prev_life_)
			tutorial_got_health_ = true;
		if(snap.bullets_remaining > tutorial_prev_ammo_)
			tutorial_got_ammo_ = true;
		if(snap.shielded)
			tutorial_got_shield_ = true;
		if(snap.firepower_seconds_left > 0)
			tutorial_got_firepower_ = true;

		tutorial_prev_life_ = snap.life;
		tutorial_prev_ammo_ = snap.bullets_remaining;

		if(tutorial_got_health_ && tutorial_got_ammo_ && tutorial_got_shield_
		   && tutorial_got_firepower_)
		{
			FinishTutorial(true);
		}
		else if(snap.pickups.empty())
		{
			// 漏掉了就重摆一排，教程不该因为手滑卡死
			tutorial_armed_ = false;
		}
		break;

	case TutorialStep::Done:
		break;
	}
}

void GameSession::FinishTutorial(bool solved)
{
	tutorial_solved_ = solved;

	if(solved && !player_settings_.tutorial_done)
	{
		player_settings_.tutorial_done = true;
		settings_->Save(player_settings_);
	}

	screen_ = Screen::TutorialDone;
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
	snapshot.tutorial_step = tutorial_step_;
	snapshot.tutorial_solved = tutorial_solved_;
	snapshot.tutorial_done = player_settings_.tutorial_done;
	snapshot.world = world_.Snapshot();
	snapshot.leaderboard = highscore_.Scores();
	snapshot.rank = rank_;
	snapshot.new_record = new_record_;
	snapshot.entered_top5 = entered_top5_;
	snapshot.final_score = final_score_;
	return snapshot;
}

} // namespace thunder
