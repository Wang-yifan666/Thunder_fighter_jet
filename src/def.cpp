// ThunderFighter 终端前端实现。
// 单一非阻塞循环：采集输入 → GameSession::Step → 按快照渲染。
// 菜单流程、暂停、结算都已迁入 thunder::GameSession，本文件不再包含阻塞循环。

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
#include "game/session_snapshot.h"

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

} // namespace

// ===================================================================
// lifecycle
// ===================================================================

ThunderFighter::ThunderFighter(): score_store_(), session_(score_store_),
                                 test_mode_(false), running_(true)
{
	ConsoleUtils::HideCursor();
	ConsoleUtils::ClearScreen();

	last_tick_ = steady_clock::now();
	last_fps_time_ = last_tick_;
}

void ThunderFighter::Run()
{
	const auto now = steady_clock::now();
	const auto dt = now - last_tick_;
	last_tick_ = now;

	session_.Step(PollRawInput(), dt);

	if(session_.exit_requested())
	{
		running_ = false;
		return;
	}

	const thunder::SessionSnapshot snapshot = session_.Snapshot();

	if(snapshot.screen == thunder::Screen::Playing)
	{
		if(!snapshot.paused)
			fps_counter_++;

		DrawPlaying(snapshot);
		scene_drawn_ = false; // 离开游戏时要立即重绘场景界面
	}
	else
	{
		const bool scene_changed =
		    !scene_drawn_ || snapshot.screen != last_scene_;
		const bool redraw_due = (now - last_scene_draw_) >= milliseconds(100);

		if(scene_changed || redraw_due)
		{
			DrawScene(snapshot);
			last_scene_ = snapshot.screen;
			last_scene_draw_ = now;
			scene_drawn_ = true;
		}
	}

	if(snapshot.screen == thunder::Screen::Playing && !snapshot.paused
	   && (now - last_fps_time_) >= 1000ms)
	{
		current_fps_ = fps_counter_;
		fps_counter_ = 0;
		last_fps_time_ = now;
	}

	ConsoleUtils::SleepMs(GameConfig::kFrameDelayMs);
}

// ===================================================================
// 场景类界面（菜单 / 难度 / 排行榜 / 结算）
// ===================================================================

void ThunderFighter::DrawScene(const thunder::SessionSnapshot& snapshot)
{
	ConsoleUtils::ClearTerminal();

	switch(snapshot.screen)
	{
	case thunder::Screen::Menu:
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
		break;

	case thunder::Screen::DifficultySelect:
		std::cout << "\n\n";
		std::cout << "================================\n";
		std::cout << "         选 择 难 度            \n";
		std::cout << "================================\n\n";

		std::cout << "  [1] 简单 (Easy)\n";
		std::cout << "  [2] 普通 (Normal)\n";
		std::cout << "  [3] 困难 (Hard)\n\n";

		std::cout << "  [B] 返回主菜单\n\n";
		std::cout << "================================\n";
		break;

	case thunder::Screen::Leaderboard:
		std::cout << "\n\n";
		std::cout << "================================\n";
		std::cout << "          排 行 榜 TOP 5         \n";
		std::cout << "================================\n\n\n";

		for(size_t i = 0; i < snapshot.leaderboard.size(); ++i)
		{
			std::cout << "  " << (i + 1) << ". " << snapshot.leaderboard[i]
			          << " 分\n";
		}

		std::cout << "\n--------------------------------\n";
		std::cout << "  [B]返回主菜单\n";
		std::cout << "================================\n";
		break;

	case thunder::Screen::GameOver:
		std::cout << "\n=== 游戏结束 ===\n";
		std::cout << "本局得分: " << snapshot.final_score << " 分\n\n";

		std::cout << "=== 历史最高分 TOP 5 ===\n";
		for(size_t i = 0; i < snapshot.leaderboard.size(); ++i)
		{
			std::cout << (i + 1) << ". " << snapshot.leaderboard[i]
			          << " 分\n";
		}

		std::cout << "\n";
		if(snapshot.rank == 1)
			std::cout << "恭喜你！创造了历史最高分！\n";
		else if(snapshot.entered_top5)
			std::cout << "恭喜你！成为历史第 " << snapshot.rank
			          << " 名！\n";
		else
			std::cout << "本次未进入历史前五名，加油！\n";

		std::cout << "\n-------------------------\n";
		std::cout << "[R] 返回主菜单\n";
		std::cout << "[Q] 退出游戏\n";
		break;

	case thunder::Screen::Playing:
		break;
	}

	std::cout << std::flush;
}

// ===================================================================
// 游戏画面（FTXUI）
// ===================================================================

void ThunderFighter::DrawPlaying(const thunder::SessionSnapshot& snapshot)
{
	if(!test_mode_)
	{
		ConsoleUtils::ClearScreen();
	}

	const thunder::WorldSnapshot& world = snapshot.world;
	const std::vector<std::string> screen_buffer =
	    CellGrid::Build(world, snapshot.paused, GameConfig::kScreenWidth,
	                    GameConfig::kScreenHeight);

	// FTXUI 渲染：将字符缓冲区转换为 Elements
	Elements lines;
	const bool inv = world.invincible;
	for(int y = 0; y < GameConfig::kScreenHeight; ++y)
	{
		Elements line_chars;
		for(int x = 0; x < GameConfig::kScreenWidth; ++x)
		{
			char c = screen_buffer[y][x];
			line_chars.push_back(RenderConsole::CellToElement(
			    c, inv, snapshot.paused, y, GameConfig::kScreenHeight));
		}
		lines.push_back(hbox(line_chars));
	}

	auto make_life = [&]() {
		Elements life_elements;
		life_elements.push_back(
		    text(RenderConsole::LifeString(world.life,
		                                   GameConfig::kMaxLifeDisplay))
		    | bold
		    | color(Color::Red));
		return hbox(life_elements);
	};

	const int best =
	    snapshot.leaderboard.empty() ? 0 : snapshot.leaderboard.front();

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
	              text(std::to_string(world.elapsed_seconds) + " 秒  ")
	                  | bold
	                  | color(Color::BlueLight),
	              text(" | level: "),
	              text(std::to_string(world.level))
	                  | bold
	                  | color(Color::Green),
	              text(" | 剩余敌人: "),
	              text(std::to_string(world.enemies.size()
	                                  + world.pending_enemies))
	                  | bold
	                  | color(Color::Red),
	              text(" | 分数: ")
	                  | bold
	                  | color(Color::Yellow),
	              text((std::stringstream()
	                    << std::setw(10) << std::setfill('0') << world.score)
	                       .str())
	                  | bold
	                  | color(Color::YellowLight),
	              text(" | 最高分: ")
	                  | bold
	                  | color(Color::CyanLight),
	              text(std::to_string(best))
	                  | bold
	                  | color(Color::CyanLight)}),
	         hbox(
	             {text(
	                  "按Q退出, P暂停, 按WASD移动, 空格开火  |  子弹: "),
	              text(std::to_string(world.bullets_remaining))
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
