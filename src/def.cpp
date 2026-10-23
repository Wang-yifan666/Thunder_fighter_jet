// ThunderFighter 终端前端实现。
// 只负责：采集输入、驱动菜单流程、把 GameWorld 快照渲染成 FTXUI 画面。
// 对局状态与更新逻辑已迁至 thunder::GameWorld（src/game/）。

#include <ftxui/dom/elements.hpp>
#include <windows.h>

#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "def.h"
#include "cell_grid.h"
#include "console_utils.h"
#include "input_console.h"
#include "render_console.h"
#include "game/game_config.h"
#include "game/input_state.h"
#include "game/world_snapshot.h"

using namespace ftxui;
using namespace std::chrono;
using namespace std::chrono_literals;

namespace
{

thunder::RawInput PollRawInput()
{
	thunder::RawInput raw;

	auto set = [&raw](thunder::Key key, int vk_code) {
		raw.down.set(thunder::KeyIndex(key),
		             ConsoleInput::IsKeyDown(vk_code));
	};

	set(thunder::Key::Up, 'W');
	set(thunder::Key::Down, 'S');
	set(thunder::Key::Left, 'A');
	set(thunder::Key::Right, 'D');
	set(thunder::Key::Fire, VK_SPACE);
	set(thunder::Key::Pause, 'P');
	set(thunder::Key::Quit, 'Q');
	set(thunder::Key::Back, 'B');
	set(thunder::Key::Restart, 'R');
	set(thunder::Key::Escape, VK_ESCAPE);

	set(thunder::Key::Digit1, '1');
	set(thunder::Key::Digit2, '2');
	set(thunder::Key::Digit3, '3');
	set(thunder::Key::Digit4, '4');
	set(thunder::Key::Digit5, '5');
	set(thunder::Key::Digit6, '6');
	set(thunder::Key::Digit7, '7');
	set(thunder::Key::Digit8, '8');
	set(thunder::Key::Digit9, '9');

	return raw;
}

/// 菜单/结算只用按住状态，不推进 InputTracker：
/// 保持"按住 Q 开局"等既有边界行为不变，边沿只在游戏帧内计算。
bool Held(const thunder::RawInput& raw, thunder::Key key)
{
	return raw.down.test(thunder::KeyIndex(key));
}

} // namespace

// ===================================================================
// lifecycle
// ===================================================================

ThunderFighter::ThunderFighter(): test_mode_(false), state_(GameState::Menu),
                                 running_(true), score_store_(),
                                 highscore_(score_store_)
{
	ConsoleUtils::HideCursor();
	ConsoleUtils::ClearScreen();

	last_fps_time_ = steady_clock::now();
	last_tick_ = last_fps_time_;

	highscore_.Load();
	// 不在构造期刷怪：开新局由 StartGame() -> GameWorld::Reset() 负责。
}

void ThunderFighter::Run()
{
	if(state_ == GameState::Menu)
	{
		ShowMenu();
		return;
	}

	while(running_ && state_ == GameState::Playing && !ShouldExit())
	{
		DrawFrame();

		if(state_ != GameState::Playing)
			return;

		ConsoleUtils::SleepMs(GameConfig::kFrameDelayMs);
	}

	if(world_.Life() > 0)
	{
		return;
	}

	// 结算
	const thunder::WorldSnapshot snapshot = world_.Snapshot();
	const int final_score = snapshot.score
	    + snapshot.bullets_remaining * GameConfig::kEndGameBulletBonus;

	highscore_.Submit(final_score);
	const auto& scores = highscore_.Scores();

	int rank = highscore_.GetRank(final_score);

	while(true)
	{
		ConsoleUtils::ClearTerminal();

		std::cout << "\n=== 游戏结束 ===\n";
		std::cout << "本局得分: " << final_score << " 分\n\n";

		std::cout << "=== 历史最高分 TOP 5 ===\n";
		for(size_t i = 0; i < scores.size(); ++i)
		{
			std::cout << (i + 1) << ". " << scores[i] << " 分\n";
		}

		std::cout << "\n";
		if(rank == 1)
			std::cout << "恭喜你！创造了历史最高分！\n";
		else if(rank <= GameConfig::kMaxHighScores)
			std::cout << "恭喜你！成为历史第 " << rank << " 名！\n";
		else
			std::cout << "本次未进入历史前五名，加油！\n";

		std::cout << "\n-------------------------\n";
		std::cout << "[R] 返回主菜单\n";
		std::cout << "[Q] 退出游戏\n";

		const thunder::RawInput raw = PollRawInput();

		if(Held(raw, thunder::Key::Restart))
		{
			state_ = GameState::Menu;
			return;
		}

		if(Held(raw, thunder::Key::Quit))
		{
			state_ = GameState::Exit;
			running_ = false;
			return;
		}

		ConsoleUtils::SleepMs(GameConfig::kMenuSleepMs);
	}
}

bool ThunderFighter::IsRunning()
{
	return running_;
}

// ===================================================================
// menu / screen flow
// ===================================================================

void ThunderFighter::ShowMenu()
{
	while(state_ == GameState::Menu)
	{
		ConsoleUtils::ClearTerminal();

		std::cout << "\n\n";
		std::cout << "================================\n";
		std::cout << "        雷 霆 战 机           \n";
		std::cout << "================================\n\n\n";

		std::cout << "  [1] 开始游戏\n";
		std::cout << "  [2] 排行榜\n";
		std::cout << "  [3] 退出游戏\n\n\n";

		std::cout << "*********************************\n\n";

		std::cout << "  按 1 / 2 / 3 进行选择...\n";

		std::cout << "================================\n\n";

		const thunder::RawInput raw = PollRawInput();

		if(Held(raw, thunder::Key::Digit1))
		{
			ConsoleUtils::SleepMs(GameConfig::kKeyDebounceMs);
			ShowDifficultyMenu();
			return;
		}

		if(Held(raw, thunder::Key::Digit2))
		{
			ConsoleUtils::SleepMs(GameConfig::kKeyDebounceMs);
			ShowLeaderboard();
		}

		if(Held(raw, thunder::Key::Digit3))
		{
			state_ = GameState::Exit;
			running_ = false;
			return;
		}

		ConsoleUtils::SleepMs(GameConfig::kMenuSleepMs);
	}
}

void ThunderFighter::ShowDifficultyMenu()
{
	while(state_ == GameState::Menu)
	{
		ConsoleUtils::ClearTerminal();

		std::cout << "\n\n";
		std::cout << "================================\n";
		std::cout << "         选 择 难 度            \n";
		std::cout << "================================\n\n";

		std::cout << "  [1] 简单 (Easy)\n";
		std::cout << "  [2] 普通 (Normal)\n";
		std::cout << "  [3] 困难 (Hard)\n\n";

		std::cout << "  [B] 返回主菜单\n\n";
		std::cout << "================================\n";

		const thunder::RawInput raw = PollRawInput();

		if(Held(raw, thunder::Key::Digit1))
		{
			StartGame(Difficulty::Easy);
			ConsoleUtils::SleepMs(GameConfig::kKeyDebounceMs);
			return;
		}

		if(Held(raw, thunder::Key::Digit2))
		{
			StartGame(Difficulty::Normal);
			ConsoleUtils::SleepMs(GameConfig::kKeyDebounceMs);
			return;
		}

		if(Held(raw, thunder::Key::Digit3))
		{
			StartGame(Difficulty::Hard);
			ConsoleUtils::SleepMs(GameConfig::kKeyDebounceMs);
			return;
		}

		if(Held(raw, thunder::Key::Back))
		{
			ConsoleUtils::SleepMs(GameConfig::kKeyDebounceMs);
			return;
		}

		ConsoleUtils::SleepMs(GameConfig::kLeaderboardSleepMs);
	}
}

void ThunderFighter::ShowLeaderboard()
{
	highscore_.Load();

	while(state_ == GameState::Menu)
	{
		ConsoleUtils::ClearTerminal();

		std::cout << "\n\n";
		std::cout << "================================\n";
		std::cout << "          排 行 榜 TOP 5         \n";
		std::cout << "================================\n\n\n";

		const auto& scores = highscore_.Scores();

		for(size_t i = 0; i < scores.size(); ++i)
		{
			std::cout << "  " << (i + 1) << ". " << scores[i]
			          << " 分\n";
		}

		std::cout << "\n--------------------------------\n";
		std::cout << "  [B]返回主菜单\n";
		std::cout << "================================\n";

		const thunder::RawInput raw = PollRawInput();

		if(Held(raw, thunder::Key::Back))
		{
			ConsoleUtils::SleepMs(GameConfig::kKeyDebounceMs);
			return;
		}
		if(Held(raw, thunder::Key::Escape))
		{
			state_ = GameState::Exit;
			running_ = false;
			return;
		}

		ConsoleUtils::SleepMs(GameConfig::kLeaderboardSleepMs);
	}
}

void ThunderFighter::Back_to_menu()
{
	is_paused_ = false;
	state_ = GameState::Menu;
}

void ThunderFighter::StartGame(Difficulty difficulty)
{
	world_.Reset(difficulty);
	is_paused_ = false;
	last_tick_ = steady_clock::now();
	state_ = GameState::Playing;
}

bool ThunderFighter::ShouldExit() const
{
	if(world_.Over())
		return true;

	return ConsoleInput::IsKeyDown(VK_ESCAPE);
}

thunder::InputState ThunderFighter::PollInput()
{
	return input_tracker_.Update(PollRawInput());
}

// ===================================================================
// main loop
// ===================================================================

void ThunderFighter::DrawFrame()
{
	const thunder::InputState input = PollInput();

	if(input.Pressed(thunder::Key::Quit))
	{
		Back_to_menu();
		ConsoleUtils::SleepMs(GameConfig::kAntiBounceDelayMs);
		return;
	}

	// 暂停逻辑：暂停期间不推进 GameWorld，时间自然冻结。
	if(input.Down(thunder::Key::Pause))
	{
		is_paused_ = !is_paused_;
		if(is_paused_)
		{
			pause_start_time_ = steady_clock::now();
		}
		else
		{
			auto now = steady_clock::now();
			last_fps_time_ += (now - pause_start_time_);
		}
		ConsoleUtils::SleepMs(200);
	}

	const auto now = steady_clock::now();
	const auto dt = now - last_tick_;
	last_tick_ = now;

	if(!is_paused_)
	{
		fps_counter_++;

		world_.Tick(thunder::GameInput::FromInput(input), dt);

		const auto fps_elapsed = now - last_fps_time_;
		if(fps_elapsed >= 1000ms)
		{
			current_fps_ = fps_counter_;
			fps_counter_ = 0;
			last_fps_time_ = now;
		}
	}

	if(state_ != GameState::Playing)
	{
		return;
	}

	if(!test_mode_)
	{
		ConsoleUtils::ClearScreen();
	}

	const thunder::WorldSnapshot snapshot = world_.Snapshot();
	const std::vector<std::string> screen_buffer =
	    CellGrid::Build(snapshot, is_paused_, GameConfig::kScreenWidth,
	                    GameConfig::kScreenHeight);

	// FTXUI 渲染：将字符缓冲区转换为 Elements
	Elements lines;
	bool inv = snapshot.invincible;
	for(int y = 0; y < GameConfig::kScreenHeight; ++y)
	{
		Elements line_chars;
		for(int x = 0; x < GameConfig::kScreenWidth; ++x)
		{
			char c = screen_buffer[y][x];
			line_chars.push_back(RenderConsole::CellToElement(
			    c, inv, is_paused_, y, GameConfig::kScreenHeight));
		}
		lines.push_back(hbox(line_chars));
	}

	auto make_life = [&]() {
		Elements life_elements;
		life_elements.push_back(
		    text(RenderConsole::LifeString(snapshot.life,
		                                   GameConfig::kMaxLifeDisplay))
		    | bold
		    | color(Color::Red));
		return hbox(life_elements);
	};

	auto window =
	    vbox(
	        {text(std::string(GAME_NAME) + " " + GAME_VERSION)
	             | bold
	             | center,
	         separator(),
	         hbox(
	             {text("帧率: "),
	              text(std::to_string(current_fps_) + " FPS  ")
	                  | bold
	                  | color(Color::Green),
	              text(" | 时长: "),
	              text(std::to_string(snapshot.elapsed_seconds) + " 秒  ")
	                  | bold
	                  | color(Color::BlueLight),
	              text(" | level: "),
	              text(std::to_string(snapshot.level))
	                  | bold
	                  | color(Color::Green),
	              text(" | 剩余敌人: "),
	              text(std::to_string(snapshot.enemies.size()
	                                  + snapshot.pending_enemies))
	                  | bold
	                  | color(Color::Red),
	              text(" | 分数: ")
	                  | bold
	                  | color(Color::Yellow),
	              text((std::stringstream()
	                    << std::setw(10) << std::setfill('0')
	                    << snapshot.score)
	                       .str())
	                  | bold
	                  | color(Color::YellowLight),
	              text(" | 最高分: ")
	                  | bold
	                  | color(Color::CyanLight),
	              text(std::to_string(highscore_.Scores().front()))
	                  | bold
	                  | color(Color::CyanLight)}),
	         hbox(
	             {text(
	                  "按Q退出, P暂停, 按WASD移动, 空格开火  |  子弹: "),
	              text(std::to_string(snapshot.bullets_remaining))
	                  | bold
	                  | color(Color::White)}),
	         separator(),
	         vbox(lines)
	             | vscroll_indicator
	             | frame,
	         filler(),
	         separator(),
	         make_life()})
	    | size(WIDTH, EQUAL, GameConfig::kWindowWidth)
	    | size(HEIGHT, EQUAL, GameConfig::kWindowHeight)
	    | border;

	auto screen = Screen::Create(
	    Dimension::Fixed(GameConfig::kWindowWidth),
	    Dimension::Fixed(GameConfig::kWindowHeight));
	Render(screen, window);
	screen.Print();
	std::cout << std::flush;
}
