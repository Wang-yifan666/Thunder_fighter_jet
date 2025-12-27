//#include <cstddef>
#include <ftxui/dom/elements.hpp>
#include <windows.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <algorithm>
//#include <random>
#include <iomanip>
//#include <fstream>

#include "def.h"

using namespace ftxui;
using namespace std::chrono;
using namespace std::chrono_literals;

namespace
{
constexpr int kScreenWidth = 98;
constexpr int kScreenHeight = 40;

int level_up_time = 10;
const int Start_to_move_down = 2;
const int Start_to_move_leftandright = 4;

int max_life_display = 10; //最大生命显示数量
} // namespace

ThunderFighter::ThunderFighter():


test_mode_(false),                //
state_(GameState::Menu),          //
frame_count_(0),                  //
running_(true),                   //
fps_counter_(0),                  //
current_fps_(0),                  //
player_x_(48),                    //
player_y_(kScreenHeight - 1),     //
life_number(5),                   //
level_(1),                        //
start_time_(steady_clock::now()), //
enemy_move_counter_(0),           //
enemy_move_interval_(20),         //
remaining_rows_in_batch_(0),      //
score_(0),                        //
top_row_clear_(true),             //
last_score_time_(0),              //
is_paused_(false),                //
is_invincible_(false),            //
q_was_down_(false),               //
invincible_until_(start_time_),   //
cheat_invincible_(false),         //
bullets_remaining_(200)           //
{
	HideCursor();
	ClearScreen();

	last_fps_time_ = steady_clock::now();

	highscore_.Load();
	Make_enermy();
}

void ThunderFighter::ShowMenu()
{
	while(state_ == GameState::Menu)
	{
		system("cls");

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

		if(GetAsyncKeyState('1') & 0x8000)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
			ShowDifficultyMenu();
			return;
		}

		if(GetAsyncKeyState('2') & 0x8000)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
			ShowLeaderboard();
		}

		if(GetAsyncKeyState('3') & 0x8000)
		{
			state_ = GameState::Exit;
			running_ = false;
			return;
		}


		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}

void ThunderFighter::ShowLeaderboard()
{
	highscore_.Load();

	while(state_ == GameState::Menu)
	{
		system("cls");

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

		if(GetAsyncKeyState('B') & 0x8000)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
			return;
		}
		if(GetAsyncKeyState(VK_ESCAPE) & 0x8000)
		{
			state_ = GameState::Exit;
			running_ = false;
			return;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(80));
	}
}

void ThunderFighter::ClearScreen() const
{
	std::cout << "\033[2J\033[H";
	std::cout << "\033[3J";
	std::cout << "\033[?1049h\033[r\033[50;1H";
	std::cout << std::flush;

	// std::cout << "\033[2J\033[H" << std::flush;
}

void ThunderFighter::HideCursor() const
{
	HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_CURSOR_INFO ci{1, FALSE};
	SetConsoleCursorInfo(h, &ci);
}

void ThunderFighter::ShowCursor() const
{
	HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_CURSOR_INFO ci{1, TRUE};
	SetConsoleCursorInfo(h, &ci);
}

bool ThunderFighter::ShouldExit() const
{
	if(life_number <= 0)
	{
		return true;
	}

	return (GetAsyncKeyState(VK_ESCAPE) & 0x8000);
}

void ThunderFighter::ResetGame()
{
	score_ = 0;
	level_ = 1;

	// 重置时间
	start_time_ = std::chrono::steady_clock::now();
	last_score_time_ = 0;
	elapsed_seconds_ = 0;

	// 清空敌人和子弹
	enemies_.clear();
	pending_enemies_.clear();
	bullets_.clear();

	switch(difficulty_)
	{
	case Difficulty::Easy:
		life_number = 8;
		bullets_remaining_ = 300;
		break;
	case Difficulty::Normal:
		life_number = 5;
		bullets_remaining_ = 200;
		break;
	case Difficulty::Hard:
		life_number = 3;
		bullets_remaining_ = 150;
		break;
	}

	// 玩家位置
	player_x_ = kScreenWidth / 2 - 1;
	player_y_ = kScreenHeight - 1;

	// 敌人生成参数
	enemy_move_counter_ = 0;
	remaining_rows_in_batch_ = 0;

	// 无敌状态
	is_invincible_ = false;
	cheat_invincible_ = false;

	// 防止暂停
	is_paused_ = false;

	// 重新生成敌人
	Make_enermy();
}

void ThunderFighter::Life()
{
	life_number = 5;
}

void ThunderFighter::Level()
{
	static int last_level_up_time = 0;

	if(elapsed_seconds_ - last_level_up_time >= level_up_time)
	{
		level_++;
		last_level_up_time = elapsed_seconds_;

		bullets_remaining_ += 10; //每升一级加10发子弹

		Make_enermy();
	}

	if(GetAsyncKeyState('9') & 0x8000)
	{
		level_++;
		Make_enermy();
		bullets_remaining_ += 10;
	}

	// 根据 level 调整敌人移动速度
	int min_interval = 5;
	int max_interval = 20;
	enemy_move_interval_ =
	    max_interval - (level_ - 1) * (max_interval - min_interval) / 9;
	enemy_move_interval_ =
	    std::max<int>(enemy_move_interval_, min_interval);
}

void ThunderFighter::score()
{
	auto now2 = steady_clock::now();
	elapsed_seconds_ =
	    duration_cast<seconds>(now2 - start_time_).count();

	//每秒增加10分
	if(elapsed_seconds_ > last_score_time_)
	{
		score_ += 10;
		last_score_time_ = elapsed_seconds_;
	}
}

void ThunderFighter::DrawFrame()
{
	bool q_down = (GetAsyncKeyState('Q') & 0x8000) != 0;
	if(q_down && !q_was_down_)
	{
		Back_to_menu();
		q_was_down_ = true;
		std::this_thread::sleep_for(
		    std::chrono::milliseconds(120)); //防止抖动
		return;                              //立刻结束这一帧
	}
	q_was_down_ = q_down;

	//暂停逻辑
	if(GetAsyncKeyState('P') & 0x8000)
	{
		is_paused_ = !is_paused_;
		if(is_paused_)
		{
			pause_start_time_ = steady_clock::now();
		}
		else
		{
			//暂停结束，修正时间，防止游戏时间跳变
			auto now = steady_clock::now();
			auto duration = now - pause_start_time_;
			start_time_ += duration;
			last_fps_time_ += duration;
			if(is_invincible_)
				invincible_until_ += duration;
		}
		std::this_thread::sleep_for(200ms); //防止按键连续触发
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
		   && now2 >= invincible_until_) //无敌计时结束检查
		{
			is_invincible_ = false;
		}

		if(enemy_move_counter_ >= enemy_move_interval_)
		{
			MoveEnemies(enemies_, level_, kScreenWidth, kScreenHeight);
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

		const int player_w = 3;
		const int player_h = 1;

		if((GetAsyncKeyState(VK_SPACE) & 0x8000)
		   && bullets_remaining_ > 0)
		{
			int bx = player_x_ + 1; //玩家 "*0*" 的中间
			int by = player_y_ - 1; //玩家上面一格

			if(by < 0)
				by = 0;

			bullets_.emplace_back(bx, by); //在列表里新增一颗子弹
			bullets_remaining_--;          //弹药 -1
		}

		if(GetAsyncKeyState('A') & 0x8000)
			player_x_--;
		if(GetAsyncKeyState('D') & 0x8000)
			player_x_++;
		if(GetAsyncKeyState('W') & 0x8000)
			player_y_--;
		if(GetAsyncKeyState('S') & 0x8000)
			player_y_++;

		if(GetAsyncKeyState('8') & 0x8000)
		{
			Clear_Current_Enemies();
		}
		if(GetAsyncKeyState('7') & 0x8000)
		{
			cheats_kills();
		}
		if(GetAsyncKeyState('6') & 0x8000)
		{
			cheats_life();
		}
		if(GetAsyncKeyState('5') & 0x8000)
		{
			cheats_invincible();
		}
		if(GetAsyncKeyState('4') & 0x8000)
		{
			cheats_addscore();
		}
		if(GetAsyncKeyState('3') & 0x8000)
		{
			cheats_addbullters();
		}

		if(GetAsyncKeyState('1') & 0x8000)
		{
			cheats_godlike();
		}

		player_x_ = std::clamp(player_x_, 0, kScreenWidth - player_w);
		player_y_ = std::clamp(player_y_, 0, kScreenHeight - player_h);

		CheckPlayerCollision();

		UpdateBullets();
	}

	if(state_ != GameState::Playing)
	{
		return;
	}

	if(!test_mode_)
	{
		ClearScreen(); //保持画面不消失
	}

	//创建字符缓冲区
	std::vector<std::string> screen_buffer(
	    kScreenHeight, std::string(kScreenWidth, ' '));

	//绘制敌人
	alive_enemies_count = 0;
	for(auto& e: enemies_)
	{
		if(!e.alive)
			continue;

		int draw_y = static_cast<int>(e.y);

		if(draw_y < 0 || draw_y >= kScreenHeight)
			continue;

		for(int i = 0; i < 3 && e.x + i < kScreenWidth; i++)
		{
			if(e.x + i >= 0)
				screen_buffer[draw_y][e.x + i] = "$o$"[i];
		}

		alive_enemies_count++;
	}

	//绘制子弹
	for(const auto& b: bullets_)
	{
		if(b.y >= 0 && b.y < kScreenHeight && b.x >= 0
		   && b.x < kScreenWidth)
		{
			screen_buffer[b.y][b.x] = '|';
		}
	}

	//绘制玩家
	std::string player_str = "*0*";
	for(int i = 0; i < 3 && player_x_ + i < kScreenWidth; ++i)
		screen_buffer[player_y_][player_x_ + i] = player_str[i];

	//暂停状态时显示暂停文字
	if(is_paused_)
	{
		std::string pause_txt = "=== P A U S E D ===";
		int py = kScreenHeight / 2;
		int px = (kScreenWidth - (int)pause_txt.size()) / 2;
		for(int i = 0; i < (int)pause_txt.size(); ++i)
		{
			if(px + i >= 0 && px + i < kScreenWidth)
				screen_buffer[py][px + i] = pause_txt[i];
		}
	}

	Elements lines;
	for(int y = 0; y < kScreenHeight; ++y)
	{
		Elements line_chars;
		for(int x = 0; x < kScreenWidth; ++x)
		{
			char c = screen_buffer[y][x];
			if(c == '*' || c == '0') // 玩家
			{
				bool inv = is_invincible_ || cheat_invincible_;
				auto col = inv ? Color::GreenLight : Color::YellowLight;
				line_chars.push_back(text(std::string(1, c)) //
				                     | bold                  //
				                     | color(col));          //
			}
			else if(c == '$' || c == 'o')                       // 敌人
				line_chars.push_back(text(std::string(1, c))    //
				                     | bold                     //
				                     | color(Color::RedLight)); //
			else if(c == '|')                                   // 子弹
				line_chars.push_back(text("|")                  //
				                     | bold                     //
				                     | color(Color::Magenta));  //
			// === 新增：暂停文字高亮显示 ===
			else if(is_paused_ && y == kScreenHeight / 2 && c != ' ')
			{
				line_chars.push_back(text(std::string(1, c)) | bold
				                     | color(Color::White));
			}
			else
				line_chars.push_back(text(" "));
		}
		lines.push_back(hbox(line_chars));
	}

	auto make_life = [&]() {
		Elements life_elements;
		std::string life_str = "剩余生命: ";
		for(int i = 1; i <= life_number; i++)
		{
			if(i >= max_life_display)
			{
				life_str += " ++ ";
				break;
			}
			else
			{
				life_str += "<> ";
			}
		}
		life_elements.push_back(text(life_str)        //
		                        | bold                //
		                        | color(Color::Red)); //
		return hbox(life_elements);
	};

	auto window =
	    vbox(
	        {text(std::string(GAME_NAME) + " " + GAME_VERSION) //
	             | bold                                        //
	             | center,                                     //
	         separator(),                                      //
	         hbox(
	             {text("帧率: "),                                  //
	              text(std::to_string(current_fps_) + " FPS  ")    //
	                  | bold                                       //
	                  | color(Color::Green),                       //
	              text(" | 时长: "),                               //
	              text(std::to_string(elapsed_seconds_) + " 秒  ") //
	                  | bold                                       //
	                  | color(Color::BlueLight),                   //
	              text(" | level: "),                              //
	              text(std::to_string(level_))                     //
	                  | bold                                       //
	                  | color(Color::Green),                       //
	              text(" | 剩余敌人: "),                           //
	              text(std::to_string(enemies_.size()              //
	                                  + pending_enemies_.size()))  //
	                  | bold                                       //
	                  | color(Color::Red),                         //
	              text(" | 分数: ")                                //
	                  | bold                                       //
	                  | color(Color::Yellow),
	              text((std::stringstream()
	                    << std::setw(10) << std::setfill('0') << score_)
	                       .str())                                  //
	                  | bold                                        //
	                  | color(Color::YellowLight),                  //
	              text(" | 最高分: ")                               //
	                  | bold                                        //
	                  | color(Color::CyanLight),                    //
	              text(std::to_string(highscore_.Scores().front())) //
	                  | bold                                        //
	                  | color(Color::CyanLight)}),                  //
	         hbox(
	             {text(
	                  "按Q退出, P暂停, 按WASD移动, 空格开火  |  子弹: "), // 更新了操作说明
	              text(std::to_string(bullets_remaining_)) //
	                  | bold                               //
	                  | color(Color::White)}),             ////
	         separator(),                                  //
	         vbox(lines)                                   //
	             | vscroll_indicator                       //
	             | frame,
	         filler(),            //
	         separator(),         //
	         make_life()})        //
	    | size(WIDTH, EQUAL, 100) //
	    | size(HEIGHT, EQUAL, 50) //
	    | border;                 //

	auto screen =
	    Screen::Create(Dimension::Fixed(100), Dimension::Fixed(50));
	Render(screen, window);
	screen.Print();
	std::cout << std::flush;
}

void ThunderFighter::CheckPlayerCollision()
{
	if(is_invincible_ || cheat_invincible_) //已经处于无敌状态，直接返回
		return;

	const int player_w = 3;
	const int player_h = 1;

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

		//矩形碰撞检测：如果完全不重叠则跳过
		bool no_overlap = (px + player_w <= ex) //
		    || (ex + ew <= px)                  //
		    || (py + player_h <= ey)            //
		    || (ey + eh <= py);                 //

		if(no_overlap)
			continue;

		//走到这里说明发生碰撞
		life_number--;                                // 扣一条命
		is_invincible_ = true;                        // 开启无敌
		invincible_until_ = steady_clock::now() + 1s; // 1 秒无敌


		if(life_number < 0) //生命耗尽，下一帧 ShouldExit() 会让游戏结束
			life_number = 0;

		break; //本帧只处理一次碰撞
	}
}

void ThunderFighter::UpdateBullets()
{
	if(bullets_.empty())
		return;

	//所有子弹向上移动一格
	for(auto& b: bullets_)
	{
		b.y -= 1;
	}

	//删除飞出屏幕顶端的子弹
	bullets_.erase(std::remove_if       //
	               (bullets_.begin(),   //
	                bullets_.end(),     //
	                [](const Bullet& b) //
	                {
		                return b.y < 0;
	                }),
	               bullets_.end());

	//子弹与敌人碰撞检测
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

			bool hit = (b_it->y >= ey)  //
			    && (b_it->y < ey + eh)  //
			    && (b_it->x >= ex)      //
			    && (b_it->x < ex + ew); //

			if(hit)
			{
				//子弹打中敌人：删敌人、删子弹、加分
				e_it = enemies_.erase(e_it);
				score_ += 100; //每个敌人击破+100分
				bullet_consumed = true;
				bullets_remaining_ += 5; //每击落一个敌人加5发子弹
				break;                   //退出本帧
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

void ThunderFighter::Run()
{
	//主菜单
	if(state_ == GameState::Menu)
	{
		ShowMenu();
		return;
	}

	//游戏状态
	while(running_ && state_ == GameState::Playing && !ShouldExit())
	{
		DrawFrame();

		//切换状态
		if(state_ != GameState::Playing)
			return;

		std::this_thread::sleep_for(std::chrono::milliseconds(16));
	}

	if(life_number > 0)
	{
		return;
	}

	//结算
	score_ += bullets_remaining_ * 10;

	// 提交分数并保存
	highscore_.Submit(score_);
	const auto& scores = highscore_.Scores();

	// 计算排名
	int rank = 1;
	for(int s: scores)
	{
		if(score_ < s)
			rank++;
		else
			break;
	}

	//结算界面
	while(true)
	{
		system("cls");

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
		else if(rank <= 5)
			std::cout << "恭喜你！成为历史第 " << rank << " 名！\n";
		else
			std::cout << "本次未进入历史前五名，加油！\n";

		std::cout << "\n-------------------------\n";
		std::cout << "[R] 返回主菜单\n";
		std::cout << "[Q] 退出游戏\n";

		if(GetAsyncKeyState('R') & 0x8000)
		{
			state_ = GameState::Menu;
			return;
		}

		if(GetAsyncKeyState('Q') & 0x8000)
		{
			state_ = GameState::Exit;
			running_ = false;
			return;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}

bool ThunderFighter::IsRunning()
{
	return running_;
}

void ThunderFighter::Back_to_menu()
{
	is_paused_ = false;
	is_invincible_ = false;
	cheat_invincible_ = false;

	state_ = GameState::Menu;
}

void ThunderFighter::ShowDifficultyMenu()
{
	while(state_ == GameState::Menu)
	{
		system("cls");

		std::cout << "\n\n";
		std::cout << "================================\n";
		std::cout << "         选 择 难 度            \n";
		std::cout << "================================\n\n";

		std::cout << "  [1] 简单 (Easy)\n";
		std::cout << "  [2] 普通 (Normal)\n";
		std::cout << "  [3] 困难 (Hard)\n\n";

		std::cout << "  [B] 返回主菜单\n\n";
		std::cout << "================================\n";

		if(GetAsyncKeyState('1') & 0x8000)
		{
			difficulty_ = Difficulty::Easy;
			ResetGame();
			state_ = GameState::Playing;
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
			return;
		}

		if(GetAsyncKeyState('2') & 0x8000)
		{
			difficulty_ = Difficulty::Normal;
			ResetGame();
			state_ = GameState::Playing;
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
			return;
		}

		if(GetAsyncKeyState('3') & 0x8000)
		{
			difficulty_ = Difficulty::Hard;
			ResetGame();
			state_ = GameState::Playing;
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
			return;
		}

		if(GetAsyncKeyState('B') & 0x8000)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
			return;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(80));
	}
}
