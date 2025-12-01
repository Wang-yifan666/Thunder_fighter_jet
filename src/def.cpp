#include <ftxui/dom/elements.hpp>
#include <windows.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <algorithm>

#include "def.h"

using namespace ftxui;
using namespace std::chrono;
using namespace std::chrono_literals;

ThunderFighter::ThunderFighter():


frame_count_(0),                 //
running_(true),                  //
fps_counter_(0),                 //
current_fps_(0),                 //
player_x_(48),                   //
player_y_(43),                   //
life_number(5),                  //
level_(1),                       //
start_time_(steady_clock::now()) //


{
	HideCursor();
	ClearScreen();
	last_fps_time_ = steady_clock::now();
}

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
	// Placeholder for enemy creation logic
}

void ThunderFighter::Level()
{
	level_ = 1 + (elapsed_seconds_ / 20);
}


void ThunderFighter::score()
{
	int score = 0;
}

void ThunderFighter::DrawFrame()
{
	frame_count_++;
	fps_counter_++;

	auto now2 = steady_clock::now(); //计算已运行时间
	elapsed_seconds_ =
	    duration_cast<seconds>(now2 - start_time_).count();

	Level();

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

	player_x_ = std::clamp(player_x_, 0, screen_width - player_w);
	player_y_ = std::clamp(player_y_, 0, screen_height - player_h);

	ClearScreen();

	Elements lines;
	{
		while(lines.size() < screen_height)
			lines.push_back(text(""));

		std::string player = "*0*";

		std::string row = std::string(player_x_, ' ') + player;

		lines[player_y_] = text(row)     //
		    | bold                       //
		    | color(Color::YellowLight); //
	}

	auto make_life = [&]() {
		Elements life_elements;
		std::string life_str = "剩余生命: ";
		for(int i = 1; i <= life_number; i++)
		{
			life_str += "<> ";
		}
		life_elements.push_back(text(life_str)        //
		                        | bold                //
		                        | color(Color::Red)); //
		return hbox(life_elements);
	};

	auto window =
	    vbox({
	        text("雷霆战机 v1.0") //
	            | bold            //
	            | center,         //
	        separator(),          //
	        hbox({
	            text("帧率: "),                                     //
	            text(std::to_string(current_fps_) + " FPS  ")       //
	                | bold                                          //
	                | color(Color::Green),                          //
	            text(" | 时长: "),                                  //
	            text(std::to_string(elapsed_seconds_) + " 秒     ") //
	                | bold                                          //
	                | color(Color::BlueLight),                      //
	            text("level: "),                                    //
	            text(std::to_string(level_))                        //
	                | bold                                          //
	                | color(Color::Green)                           //
	        }),

	        text("按q退出,按wasd移动"), //
	        separator(),                //
	        vbox(lines)                 //
	            | vscroll_indicator     //
	            | frame,                //
	        filler(),                   //
	        separator(),                //
	        make_life()                 //
	    })                              //
	    | size(WIDTH, EQUAL, 100)       //
	    | size(HEIGHT, EQUAL, 50)       //
	    | border;                       //

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
