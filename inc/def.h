#pragma once

#include <chrono>

#include "HighScore.h"
#include "game/game_config.h"
#include "game/game_types.h"
#include "game/game_world.h"
#include "game/input_state.h"

constexpr const char* GAME_NAME = "雷霆战机";
constexpr const char* GAME_VERSION = "v1.4";

/// 终端前端：输入采集、流程菜单与 FTXUI 渲染。
/// 对局状态与更新逻辑在 thunder::GameWorld 中，菜单流程仍在终端层（B5 起抽到 GameSession）。
class ThunderFighter
{
public:
	// ========== lifecycle ==========
	ThunderFighter();
	~ThunderFighter() = default;

	void Run();       // 主循环
	bool IsRunning(); // 检查游戏是否在运行

	// ========== test helpers ==========
	GameState GetState() const { return state_; }
	int GetLife() const { return world_.Life(); }
	int GetScore() const { return world_.Score(); }
	bool IsRunning() const { return running_; }

private:
	// ========== menu / screen flow ==========
	void ShowMenu();
	void ShowDifficultyMenu();
	void ShowLeaderboard();
	void Back_to_menu();
	void StartGame(Difficulty difficulty);
	bool ShouldExit() const;

	// ========== main loop ==========
	void DrawFrame();
	thunder::InputState PollInput();

	// ========== state ==========
	bool test_mode_;
	GameState state_;
	bool running_;

	// ========== world / storage ==========
	thunder::GameWorld world_;
	HighScore highscore_;

	// ========== terminal timing / display ==========
	bool is_paused_ = false;
	std::chrono::steady_clock::time_point pause_start_time_;
	std::chrono::steady_clock::time_point last_tick_;
	std::chrono::steady_clock::time_point last_fps_time_;
	int fps_counter_ = 0;
	int current_fps_ = 0;

	// ========== input ==========
	thunder::InputTracker input_tracker_;
};
