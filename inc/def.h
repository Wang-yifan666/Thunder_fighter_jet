#pragma once

#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>
#include <chrono>
#include <vector>

#include "enemy.h"
#include "HighScore.h"
#include "game_types.h"

constexpr const char* GAME_NAME = "雷霆战机";
constexpr const char* GAME_VERSION = "v1.4";

class ThunderFighter
{
public:
	// ========== lifecycle ==========
	ThunderFighter();
	~ThunderFighter() = default;

	void Run();          // 主循环
	bool IsRunning();    // 检查游戏是否在运行

	// ========== test helpers ==========
	GameState GetState() const { return state_; }
	int GetLife() const  { return life_number; }
	int GetScore() const { return score_; }
	bool IsRunning() const { return running_; }

private:
	// ========== state ==========
	bool test_mode_;
	GameState state_;
	bool running_;

	// ========== state: difficulty / level / score / time ==========
	Difficulty difficulty_ = Difficulty::Easy;
	int level_;
	int score_;
	int elapsed_seconds_ = 0;
	int last_score_time_ = 0;
	int frame_count_;
	int fps_counter_;
	int current_fps_;
	std::chrono::steady_clock::time_point start_time_;
	std::chrono::steady_clock::time_point last_fps_time_;
	std::chrono::steady_clock::time_point pause_start_time_;
	bool is_paused_ = false;
	bool top_row_clear_;

	// ========== menu / screen flow ==========
	void ShowMenu();
	void ShowDifficultyMenu();
	void ShowLeaderboard();
	void Back_to_menu();
	void ResetGame();
	bool ShouldExit() const;

	// ========== main loop helpers ==========
	void DrawFrame();
	void Level();
	void score();

	// ========== player ==========
	int player_x_;
	int player_y_;
	int life_number;

	bool is_invincible_ = false;
	bool cheat_invincible_ = false;
	bool q_was_down_ = false;
	std::chrono::steady_clock::time_point invincible_until_;

	void Life();
	void CheckPlayerCollision();

	// ========== bullets ==========
	struct Bullet
	{
		int x;
		int y;
		Bullet(int _x = 0, int _y = 0): x(_x), y(_y) {}
	};
	std::vector<Bullet> bullets_;
	int bullets_remaining_;
	void UpdateBullets();

	// ========== enemies ==========
	std::vector<Enemy> enemies_;
	std::vector<Enemy> pending_enemies_;
	int alive_enemies_count = 0;
	int enemy_move_counter_;
	int enemy_move_interval_;
	int remaining_rows_in_batch_;

	void Make_enermy();
	void Clear_Current_Enemies();
	void SpawnEnemiesFromPending();
	bool IsOverlapping(const Enemy& a, const Enemy& b);
	bool WillOverlap(const Enemy& cur,
	                 const std::vector<Enemy>& enemies, int new_x,
	                 int new_y);
	void MoveEnemies(std::vector<Enemy>& active_enemies, int level,
	                 int screen_width, int screen_height);

	// ========== high score ==========
	HighScore highscore_;

	// ========== console / rendering helpers ==========
	void ClearScreen() const;
	void HideCursor() const;
	void ShowCursor() const;

	// ========== cheats ==========
	void cheats_kills();
	void cheats_life();
	void cheats_invincible();
	void cheats_addscore();
	void cheats_addbullters();
	void cheats_godlike();
	void kill_self();
};