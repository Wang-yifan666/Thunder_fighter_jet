// ThunderFighter 实现
// ========== lifecycle ==========
// ========== menu / screen flow ==========
// ========== main loop helpers ==========
// ========== player ==========
// ========== bullets ==========
// ========== enemies ==========
// ========== console helpers ==========
// ========== cheats ==========

#include <ftxui/dom/elements.hpp>
#include <windows.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <algorithm>
#include <iomanip>

#include "def.h"
#include "game_config.h"
#include "console_utils.h"
#include "input_console.h"
#include "render_console.h"

using namespace ftxui;
using namespace std::chrono;
using namespace std::chrono_literals;

// ===================================================================
// lifecycle
// ===================================================================

ThunderFighter::ThunderFighter():
test_mode_(false),
state_(GameState::Menu),
frame_count_(0),
running_(true),
fps_counter_(0),
current_fps_(0),
player_x_(48),
player_y_(GameConfig::kScreenHeight - 1),
life_number(GameConfig::kInitialLife),
level_(1),
start_time_(steady_clock::now()),
enemy_move_counter_(0),
enemy_move_interval_(GameConfig::kInitialEnemyMoveInterval),
remaining_rows_in_batch_(0),
score_(0),
top_row_clear_(true),
last_score_time_(0),
is_paused_(false),
is_invincible_(false),
q_was_down_(false),
invincible_until_(start_time_),
cheat_invincible_(false),
bullets_remaining_(GameConfig::kInitialBullets)
{
	HideCursor();
	ClearScreen();

	last_fps_time_ = steady_clock::now();

	highscore_.Load();
	Make_enermy();
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

	if(life_number > 0)
	{
		return;
	}

	// 结算
	score_ += bullets_remaining_ * GameConfig::kEndGameBulletBonus;

	highscore_.Submit(score_);
	const auto& scores = highscore_.Scores();

	int rank = highscore_.GetRank(score_);

	while(true)
	{
		ConsoleUtils::ClearTerminal();

		std::cout << "\n=== 游戏结束 ===\n";
		std::cout << "本局得分: " << score_ << " 分\n\n";

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

		if(ConsoleInput::IsKeyDown('R'))
		{
			state_ = GameState::Menu;
			return;
		}

		if(ConsoleInput::IsKeyDown('Q'))
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

		if(ConsoleInput::IsKeyDown('1'))
		{
			ConsoleUtils::SleepMs(GameConfig::kKeyDebounceMs);
			ShowDifficultyMenu();
			return;
		}

		if(ConsoleInput::IsKeyDown('2'))
		{
			ConsoleUtils::SleepMs(GameConfig::kKeyDebounceMs);
			ShowLeaderboard();
		}

		if(ConsoleInput::IsKeyDown('3'))
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

		if(ConsoleInput::IsKeyDown('1'))
		{
			difficulty_ = Difficulty::Easy;
			ResetGame();
			state_ = GameState::Playing;
			ConsoleUtils::SleepMs(GameConfig::kKeyDebounceMs);
			return;
		}

		if(ConsoleInput::IsKeyDown('2'))
		{
			difficulty_ = Difficulty::Normal;
			ResetGame();
			state_ = GameState::Playing;
			ConsoleUtils::SleepMs(GameConfig::kKeyDebounceMs);
			return;
		}

		if(ConsoleInput::IsKeyDown('3'))
		{
			difficulty_ = Difficulty::Hard;
			ResetGame();
			state_ = GameState::Playing;
			ConsoleUtils::SleepMs(GameConfig::kKeyDebounceMs);
			return;
		}

		if(ConsoleInput::IsKeyDown('B'))
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

		if(ConsoleInput::IsKeyDown('B'))
		{
			ConsoleUtils::SleepMs(GameConfig::kKeyDebounceMs);
			return;
		}
		if(ConsoleInput::IsKeyDown(VK_ESCAPE))
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
	is_invincible_ = false;
	cheat_invincible_ = false;

	state_ = GameState::Menu;
}

void ThunderFighter::ResetGame()
{
	score_ = 0;
	level_ = 1;

	start_time_ = std::chrono::steady_clock::now();
	last_score_time_ = 0;
	elapsed_seconds_ = 0;

	enemies_.clear();
	pending_enemies_.clear();
	bullets_.clear();

	switch(difficulty_)
	{
	case Difficulty::Easy:
		life_number = GameConfig::kEasyLife;
		bullets_remaining_ = GameConfig::kEasyBullets;
		break;
	case Difficulty::Normal:
		life_number = GameConfig::kNormalLife;
		bullets_remaining_ = GameConfig::kNormalBullets;
		break;
	case Difficulty::Hard:
		life_number = GameConfig::kHardLife;
		bullets_remaining_ = GameConfig::kHardBullets;
		break;
	}

	player_x_ = GameConfig::kScreenWidth / 2 - 1;
	player_y_ = GameConfig::kScreenHeight - 1;

	enemy_move_counter_ = 0;
	remaining_rows_in_batch_ = 0;

	is_invincible_ = false;
	cheat_invincible_ = false;

	is_paused_ = false;

	Make_enermy();
}

bool ThunderFighter::ShouldExit() const
{
	if(life_number <= 0)
	{
		return true;
	}

	return ConsoleInput::IsKeyDown(VK_ESCAPE);
}

// ===================================================================
// main loop helpers
// ===================================================================

void ThunderFighter::DrawFrame()
{
	bool q_down = ConsoleInput::IsKeyDown('Q');
	if(q_down && !q_was_down_)
	{
		Back_to_menu();
		q_was_down_ = true;
		ConsoleUtils::SleepMs(GameConfig::kAntiBounceDelayMs);
		return;
	}
	q_was_down_ = q_down;

	// 暂停逻辑
	if(ConsoleInput::IsKeyDown('P'))
	{
		is_paused_ = !is_paused_;
		if(is_paused_)
		{
			pause_start_time_ = steady_clock::now();
		}
		else
		{
			auto now = steady_clock::now();
			auto duration = now - pause_start_time_;
			start_time_ += duration;
			last_fps_time_ += duration;
			if(is_invincible_)
				invincible_until_ += duration;
		}
		ConsoleUtils::SleepMs(200);
	}

	if(!is_paused_)
	{
		frame_count_++;
		fps_counter_++;
		enemy_move_counter_++;

		auto now2 = steady_clock::now();
		elapsed_seconds_ =
		    duration_cast<seconds>(now2 - start_time_).count();

		if(is_invincible_
		   && now2 >= invincible_until_)
		{
			is_invincible_ = false;
		}

		if(enemy_move_counter_ >= enemy_move_interval_)
		{
			MoveEnemies(enemies_, level_, GameConfig::kScreenWidth,
			            GameConfig::kScreenHeight);
			enemy_move_counter_ = 0;
		}

		if(!pending_enemies_.empty())
		{
			SpawnEnemiesFromPending();
		}

		Level();
		score();

		auto now = steady_clock::now();
		auto elapsed =
		    duration_cast<milliseconds>(now - last_fps_time_);
		if(elapsed >= 1000ms)
		{
			current_fps_ = fps_counter_;
			fps_counter_ = 0;
			last_fps_time_ = now;
		}

		const int player_w = GameConfig::kPlayerWidth;
		const int player_h = GameConfig::kPlayerHeight;

		if(ConsoleInput::IsKeyDown(VK_SPACE)
		   && bullets_remaining_ > 0)
		{
			int bx = player_x_ + 1;
			int by = player_y_ - 1;

			if(by < 0)
				by = 0;

			bullets_.emplace_back(bx, by);
			bullets_remaining_--;
		}

		if(ConsoleInput::IsKeyDown('A'))
			player_x_--;
		if(ConsoleInput::IsKeyDown('D'))
			player_x_++;
		if(ConsoleInput::IsKeyDown('W'))
			player_y_--;
		if(ConsoleInput::IsKeyDown('S'))
			player_y_++;

		if(ConsoleInput::IsKeyDown('8'))
		{
			Clear_Current_Enemies();
		}
		if(ConsoleInput::IsKeyDown('7'))
		{
			cheats_kills();
		}
		if(ConsoleInput::IsKeyDown('6'))
		{
			cheats_life();
		}
		if(ConsoleInput::IsKeyDown('5'))
		{
			cheats_invincible();
		}
		if(ConsoleInput::IsKeyDown('4'))
		{
			cheats_addscore();
		}
		if(ConsoleInput::IsKeyDown('3'))
		{
			cheats_addbullters();
		}
		if(ConsoleInput::IsKeyDown('2'))
		{
			kill_self();
		}

		if(ConsoleInput::IsKeyDown('1'))
		{
			cheats_godlike();
		}

		player_x_ = std::clamp(player_x_, 0,
		                       GameConfig::kScreenWidth - player_w);
		player_y_ = std::clamp(player_y_, 0,
		                       GameConfig::kScreenHeight - player_h);

		CheckPlayerCollision();

		UpdateBullets();
	}

	if(state_ != GameState::Playing)
	{
		return;
	}

	if(!test_mode_)
	{
		ClearScreen();
	}

	// 创建字符缓冲区
	std::vector<std::string> screen_buffer(
	    GameConfig::kScreenHeight,
	    std::string(GameConfig::kScreenWidth, ' '));

	// 绘制敌人
	alive_enemies_count = 0;
	for(auto& e: enemies_)
	{
		if(!e.alive)
			continue;

		int draw_y = static_cast<int>(e.y);

		if(draw_y < 0 || draw_y >= GameConfig::kScreenHeight)
			continue;

		for(int i = 0;
		    i < GameConfig::kEnemyWidth && e.x + i < GameConfig::kScreenWidth;
		    i++)
		{
			if(e.x + i >= 0)
				screen_buffer[draw_y][e.x + i] = "$o$"[i];
		}

		alive_enemies_count++;
	}

	// 绘制子弹
	for(const auto& b: bullets_)
	{
		if(b.y >= 0 && b.y < GameConfig::kScreenHeight && b.x >= 0
		   && b.x < GameConfig::kScreenWidth)
		{
			screen_buffer[b.y][b.x] = GameConfig::kBulletChar;
		}
	}

	// 绘制玩家
	std::string player_str = "*0*";
	for(int i = 0;
	    i < GameConfig::kPlayerWidth && player_x_ + i < GameConfig::kScreenWidth;
	    ++i)
		screen_buffer[player_y_][player_x_ + i] = player_str[i];

	// 暂停状态时显示暂停文字
	if(is_paused_)
	{
		std::string pause_txt = "=== P A U S E D ===";
		int py = GameConfig::kScreenHeight / 2;
		int px =
		    (GameConfig::kScreenWidth - (int)pause_txt.size()) / 2;
		for(int i = 0; i < (int)pause_txt.size(); ++i)
		{
			if(px + i >= 0 && px + i < GameConfig::kScreenWidth)
				screen_buffer[py][px + i] = pause_txt[i];
		}
	}

	// FTXUI 渲染：将字符缓冲区转换为 Elements
	Elements lines;
	bool inv = is_invincible_ || cheat_invincible_;
	for(int y = 0; y < GameConfig::kScreenHeight; ++y)
	{
		Elements line_chars;
		for(int x = 0; x < GameConfig::kScreenWidth; ++x)
		{
			char c = screen_buffer[y][x];
			line_chars.push_back(RenderConsole::CellToElement(
			    c, inv, is_paused_, y,
			    GameConfig::kScreenHeight));
		}
		lines.push_back(hbox(line_chars));
	}

	auto make_life = [&]() {
		Elements life_elements;
		life_elements.push_back(
		    text(RenderConsole::LifeString(life_number,
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
	              text(std::to_string(elapsed_seconds_) + " 秒  ")
	                  | bold
	                  | color(Color::BlueLight),
	              text(" | level: "),
	              text(std::to_string(level_))
	                  | bold
	                  | color(Color::Green),
	              text(" | 剩余敌人: "),
	              text(std::to_string(enemies_.size()
	                                  + pending_enemies_.size()))
	                  | bold
	                  | color(Color::Red),
	              text(" | 分数: ")
	                  | bold
	                  | color(Color::Yellow),
	              text((std::stringstream()
	                    << std::setw(10) << std::setfill('0') << score_)
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
	              text(std::to_string(bullets_remaining_))
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

void ThunderFighter::Level()
{
	static int last_level_up_time = 0;

	if(elapsed_seconds_ - last_level_up_time >= GameConfig::kLevelUpTime)
	{
		level_++;
		last_level_up_time = elapsed_seconds_;

		bullets_remaining_ += GameConfig::kBulletsPerLevelUp;

		Make_enermy();
	}

	if(ConsoleInput::IsKeyDown('9'))
	{
		level_++;
		Make_enermy();
		bullets_remaining_ += GameConfig::kBulletsPerLevelUp;
	}

	int min_interval = GameConfig::kMinEnemyMoveInterval;
	int max_interval = GameConfig::kMaxEnemyMoveInterval;
	enemy_move_interval_ =
	    max_interval
	    - (level_ - 1) * (max_interval - min_interval)
	          / GameConfig::kLevelMoveIntervalDivisor;
	enemy_move_interval_ =
	    std::max<int>(enemy_move_interval_, min_interval);
}

void ThunderFighter::score()
{
	auto now2 = steady_clock::now();
	elapsed_seconds_ =
	    duration_cast<seconds>(now2 - start_time_).count();

	if(elapsed_seconds_ > last_score_time_)
	{
		score_ += GameConfig::kScorePerSecond;
		last_score_time_ = elapsed_seconds_;
	}
}

// ===================================================================
// player
// ===================================================================

void ThunderFighter::Life()
{
	life_number = GameConfig::kInitialLife;
}

void ThunderFighter::CheckPlayerCollision()
{
	if(is_invincible_ || cheat_invincible_)
		return;

	const int player_w = GameConfig::kPlayerWidth;
	const int player_h = GameConfig::kPlayerHeight;

	int px = player_x_;
	int py = player_y_;

	for(auto& e: enemies_)
	{
		if(!e.alive)
			continue;

		int ex = e.x;
		int ey = static_cast<int>(e.y);
		int ew = e.width;
		int eh = e.height;

		bool no_overlap = (px + player_w <= ex)
		    || (ex + ew <= px)
		    || (py + player_h <= ey)
		    || (ey + eh <= py);

		if(no_overlap)
			continue;

		life_number--;
		is_invincible_ = true;
		invincible_until_ = steady_clock::now()
		                    + std::chrono::seconds(
		                        GameConfig::kInvincibleDurationSec);

		if(life_number < 0)
			life_number = 0;

		break;
	}
}

// ===================================================================
// bullets
// ===================================================================

void ThunderFighter::UpdateBullets()
{
	if(bullets_.empty())
		return;

	for(auto& b: bullets_)
	{
		b.y -= 1;
	}

	bullets_.erase(std::remove_if
	               (bullets_.begin(),
	                bullets_.end(),
	                [](const Bullet& b)
	                {
		                return b.y < 0;
	                }),
	               bullets_.end());

	for(auto b_it = bullets_.begin(); b_it != bullets_.end();)
	{
		bool bullet_consumed = false;

		for(auto e_it = enemies_.begin(); e_it != enemies_.end();)
		{
			Enemy& e = *e_it;
			if(!e.alive)
			{
				e_it = enemies_.erase(e_it);
				continue;
			}

			int ex = e.x;
			int ey = e.y;
			int ew = e.width;
			int eh = e.height;

			bool hit = (b_it->y >= ey)
			    && (b_it->y < ey + eh)
			    && (b_it->x >= ex)
			    && (b_it->x < ex + ew);

			if(hit)
			{
				e_it = enemies_.erase(e_it);
				score_ += GameConfig::kScorePerKill;
				bullet_consumed = true;
				bullets_remaining_ += GameConfig::kBulletsPerKill;
				break;
			}
			else
			{
				e_it++;
			}
		}

		if(bullet_consumed)
		{
			b_it = bullets_.erase(b_it);
		}
		else
		{
			b_it++;
		}
	}
}

// ===================================================================
// console helpers
// ===================================================================

void ThunderFighter::ClearScreen() const
{
	ConsoleUtils::ClearScreen();
}

void ThunderFighter::HideCursor() const
{
	ConsoleUtils::HideCursor();
}

void ThunderFighter::ShowCursor() const
{
	ConsoleUtils::ShowCursor();
}