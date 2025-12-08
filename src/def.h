#pragma once

#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>
//#include <string>
#include <chrono>
#include <vector>
//#include <random>


class ThunderFighter
{
public:
	ThunderFighter();
	~ThunderFighter() = default;

	void Run();

private:
	struct Enemy
	{
		int x;
		int y;
		int width;
		int height;
		int speed;
		bool alive;

		Enemy(int _x = 0, int _y = 0, int _speed = 1, int _width = 3,
		      int _height = 1):
		x(_x), y(_y), speed(_speed), width(_width), height(_height),
		alive(true)
		{
		}
	};


	std::vector<Enemy> enemies_;         //屏幕上的敌人
	std::vector<Enemy> pending_enemies_; //待生成敌人队列

	int alive_enemies_count = 0;

	void Make_enermy();

	void ClearScreen() const;
	void HideCursor() const;
	bool ShouldExit() const;
	void Life();
	void Level();
	void score();
	void ClearCurrentEnemies();
	void SpawnEnemiesFromPending();
	void DrawFrame();

	bool IsOverlapping(const Enemy& a,
	                   const Enemy& b); //检查两个敌人是否重叠
	bool WillOverlap(const Enemy& cur,
	                 const std::vector<Enemy>& enemies, int new_x,
	                 int new_y);
	void MoveEnemies(std::vector<Enemy>& active_enemies, int level,
	                 int screen_width, int screen_height);


	int frame_count_;
	int score_;
	int last_score_time_ = 0; // 上次加分时间
	bool running_;
	bool top_row_clear_;

	std::chrono::steady_clock::time_point last_fps_time_;

	int fps_counter_;
	int current_fps_;

	int player_x_;
	int player_y_;

	int life_number;

	int level_;

	int enemy_move_counter_;
	int enemy_move_interval_;

	int remaining_rows_in_batch_;

	std::chrono::steady_clock::time_point start_time_;
	int elapsed_seconds_ = 0;
};
