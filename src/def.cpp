#include <ftxui/dom/elements.hpp>
#include <windows.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <algorithm>
#include <random>
#include <iomanip>

#include "def.h"

using namespace ftxui;
using namespace std::chrono;
using namespace std::chrono_literals;

ThunderFighter::ThunderFighter():


frame_count_(0),                  //
running_(true),                   //
fps_counter_(0),                  //
current_fps_(0),                  //
player_x_(48),                    //
player_y_(43),                    //
life_number(5),                   //
level_(1),                        //
start_time_(steady_clock::now()), //
enemy_move_counter_(0),           //
enemy_move_interval_(20),         //
remaining_rows_in_batch_(0),      //
score_(0),                        //
top_row_clear_(true),             //
last_score_time_(0)               //


{
	HideCursor();
	ClearScreen();
	last_fps_time_ = steady_clock::now();

	Make_enermy();
}

//调试变量
int level_up_time = 10;
const int screen_width = 98;
const int screen_height = 40;
const int Start_to_move_down = 2;
const int Start_tomove_leftandright = 4;

void ThunderFighter::ClearScreen() const
{
	std::cout << "\033[2J\033[H";
	std::cout << "\033[3J";
	std::cout << "\033[?1049h\033[r\033[50;1H";
	std::cout << std::flush;
}

void ThunderFighter::HideCursor() const
{
	HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_CURSOR_INFO ci{1, FALSE};
	SetConsoleCursorInfo(h, &ci);
}

bool ThunderFighter::ShouldExit() const
{
	if(life_number <= 0)
	{
		return true;
	}

	return (GetAsyncKeyState('Q') & 0x8000)
	    || (GetAsyncKeyState(VK_ESCAPE) & 0x8000);
}

void ThunderFighter::Life()
{
	life_number = 5;
}

void ThunderFighter::Make_enermy()
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<int> randSpeed(1, 3);

	int total_to_spawn = level_ * 2;

	for(int i = 0; i < total_to_spawn; ++i)
		pending_enemies_.emplace_back(randSpeed(gen));
}

void ThunderFighter::Level()
{
	static int last_level_up_time = 0;

	if(elapsed_seconds_ - last_level_up_time >= level_up_time)
	{
		level_++;
		last_level_up_time = elapsed_seconds_;

		Make_enermy();
	}

	if(GetAsyncKeyState('9') & 0x8000)
	{
		level_++;
		Make_enermy();
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

void ThunderFighter::ClearCurrentEnemies()
{
	enemies_.clear();
	alive_enemies_count = 0;
}

void ThunderFighter::SpawnEnemiesFromPending()
{
	if(pending_enemies_.empty())
		return;

	std::random_device rd;
	std::mt19937 gen(rd());

	const int enemy_width = 3;
	const int max_x = screen_width - enemy_width;

	// 如果没有剩余行，生成新批次
	if(remaining_rows_in_batch_ == 0)
	{
		int max_rows =
		    std::min<int>(15, (int)((pending_enemies_.size() + 2) / 3));
		std::uniform_int_distribution<int> row_dist(1, max_rows);
		remaining_rows_in_batch_ = row_dist(gen);
	}

	// 尝试生成多行
	while(remaining_rows_in_batch_ > 0)
	{
		// 判断顶部是否空
		bool top_row_clear = std::none_of(
		    enemies_.begin(), enemies_.end(), [](const Enemy& e) {
			    return e.y < 1;
		    });
		if(!top_row_clear)
			break; // 顶部还没空，停止生成下一行

		int remaining = pending_enemies_.size();
		if(remaining <= 0)
			break;

		int max_in_row = std::min<int>(3, remaining);
		std::uniform_int_distribution<int> count_dist(1, max_in_row);
		int enemies_in_row = count_dist(gen);

		std::vector<int> used_x;
		std::vector<Enemy> spawned_row;

		for(int i = 0; i < enemies_in_row; ++i)
		{
			int x, attempts = 0;
			do
			{
				x = std::uniform_int_distribution<int>(0, max_x)(gen);
				attempts++;
			} while(std::any_of(used_x.begin(), used_x.end(),
			                    [&](int ux) {
				                    return std::abs(ux - x)
				                        < enemy_width;
			                    })
			        && attempts < 100);

			used_x.push_back(x);

			Enemy e = pending_enemies_.front();
			pending_enemies_.erase(pending_enemies_.begin());

			e.x = x;
			e.y = 0;
			spawned_row.push_back(e);
		}

		enemies_.insert(enemies_.end(), spawned_row.begin(),
		                spawned_row.end());
		remaining_rows_in_batch_--;
	}
}

bool ThunderFighter::IsOverlapping(const Enemy& a, const Enemy& b)
{
	return !(a.x + a.width <= b.x     //
	         || b.x + b.width <= a.x  //
	         || a.y + a.height <= b.y //
	         || b.y + b.height <= a.y);
}

bool ThunderFighter::WillOverlap(const Enemy& cur,
                                 const std::vector<Enemy>& enemies,
                                 int new_x, int new_y)
{
	Enemy temp = cur;
	temp.x = new_x;
	temp.y = new_y;
	for(auto& e: enemies)
	{
		if(&e == &cur)
			continue;
		if(IsOverlapping(temp, e))
			return true;
	}
	return false;
}

void ThunderFighter::MoveEnemies(std::vector<Enemy>& active_enemies,
                                 int level, int screen_width,
                                 int screen_height)
{
	std::vector<Enemy> to_pending; // 到底部的敌人

	for(auto it = active_enemies.begin(); it != active_enemies.end();)
	{
		// 向下移动
		it->y += it->speed;

		if(it->y >= screen_height) // 到底部
		{
			// 放回 pending
			to_pending.push_back(*it);
			it = active_enemies.erase(it);
		}
		else
		{
			// 水平移动
			if(level >= Start_tomove_leftandright)
			{
				int dir = rand() % 3 - 1; // -1 左, 0 不动, 1 右
				int new_x = it->x + dir;
				if(new_x < 0)
					new_x = 0;
				if(new_x + it->width >= screen_width)
					new_x = screen_width - it->width;
				it->x = new_x;
			}
			++it;
		}
	}

	// 放回 pending
	pending_enemies_.insert(pending_enemies_.end(), to_pending.begin(),
	                        to_pending.end());
}

void ThunderFighter::DrawFrame()
{
	frame_count_++;
	fps_counter_++;
	enemy_move_counter_++;

	auto now2 = steady_clock::now();
	elapsed_seconds_ =
	    duration_cast<seconds>(now2 - start_time_).count();

	if(enemy_move_counter_ >= enemy_move_interval_)
	{
		MoveEnemies(enemies_, level_, screen_width, screen_height);
		enemy_move_counter_ = 0;
	}

	if(enemies_.empty() && !pending_enemies_.empty())
	{
		SpawnEnemiesFromPending();
	}

	Level();
	score();

	auto now = steady_clock::now();
	auto elapsed = duration_cast<milliseconds>(now - last_fps_time_);
	if(elapsed >= 1000ms)
	{
		current_fps_ = fps_counter_;
		fps_counter_ = 0;
		last_fps_time_ = now;
	}

	const int player_w = 3;
	const int player_h = 1;
	const int screen_width = 98;
	const int screen_height = 40;

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
		ClearCurrentEnemies();
	}

	player_x_ = std::clamp(player_x_, 0, screen_width - player_w);
	player_y_ = std::clamp(player_y_, 0, screen_height - player_h);

	ClearScreen();

	//创建字符缓冲区
	std::vector<std::string> screen_buffer(
	    screen_height, std::string(screen_width, ' '));

	//绘制敌人
	alive_enemies_count = 0;
	for(auto& e: enemies_)
	{
		if(!e.alive)
			continue;

		int draw_y = int(e.y);
		for(int i = 0; i < 3 && e.x + i < screen_width; i++)
			screen_buffer[e.y][e.x + i] = "$o$"[i];

		alive_enemies_count++;
	}

	std::string player_str = "*0*";
	for(int i = 0; i < 3 && player_x_ + i < screen_width; ++i)
		screen_buffer[player_y_][player_x_ + i] = player_str[i];

	Elements lines;
	for(int y = 0; y < screen_height; ++y)
	{
		Elements line_chars;
		for(int x = 0; x < screen_width; ++x)
		{
			char c = screen_buffer[y][x];
			if(c == '*' || c == '0')                         // 玩家
				line_chars.push_back(text(std::string(1, c)) //
				                     | bold                  //
				                     | color(Color::YellowLight)); //
			else if(c == '$' || c == 'o')                       // 敌人
				line_chars.push_back(text(std::string(1, c))    //
				                     | bold                     //
				                     | color(Color::RedLight)); //
			else
				line_chars.push_back(text(" "));
		}
		lines.push_back(hbox(line_chars));
	}

	auto make_life = [&]() {
		Elements life_elements;
		std::string life_str = "剩余生命: ";
		for(int i = 1; i <= life_number; i++)
			life_str += "<> ";
		life_elements.push_back(text(life_str)        //
		                        | bold                //
		                        | color(Color::Red)); //
		return hbox(life_elements);
	};

	auto window =
	    vbox({text("雷霆战机 v1.1") //
	              | bold            //
	              | center,         //
	          separator(),          //
	          hbox({
	              text("帧率: "),                                  //
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
	                       .str())                //
	                  | bold                      //
	                  | color(Color::YellowLight) //
	          }),                                 //
	          text("按q退出,按wasd移动"),         //
	          separator(),                        //
	          vbox(lines)                         //
	              | vscroll_indicator             //
	              | frame,
	          filler(),           //
	          separator(),        //
	          make_life()})       //
	    | size(WIDTH, EQUAL, 100) //
	    | size(HEIGHT, EQUAL, 50) //
	    | border;                 //

	auto screen =
	    Screen::Create(Dimension::Fixed(100), Dimension::Fixed(50));
	Render(screen, window);
	screen.Print();
	std::cout << std::flush;
}


void ThunderFighter::Run()
{

	while(running_ && !ShouldExit())
	{
		DrawFrame();
		std::this_thread::sleep_for(16ms);
	}

	ClearScreen();
	std::cout << "\n\n雷霆战机已退出,平均帧率: " << current_fps_
	          << " FPS" << std::endl;
	system("pause");
}
