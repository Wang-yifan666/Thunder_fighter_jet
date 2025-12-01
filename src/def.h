#pragma once

#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>
//#include <string>
#include <chrono>


class ThunderFighter
{
public:
	ThunderFighter();
	~ThunderFighter() = default;

	void Run();

private:
	void ClearScreen() const;
	void HideCursor() const;
	bool ShouldExit() const;
	void Life();
	void Make_enermy();
	void Level();
	void score();
	void DrawFrame();

	int frame_count_;
	bool running_;

	std::chrono::steady_clock::time_point last_fps_time_;

	int fps_counter_;
	int current_fps_;

	int player_x_;
	int player_y_;

	int life_number;

	int level_;

	std::chrono::steady_clock::time_point start_time_;
	int elapsed_seconds_ = 0;
};
