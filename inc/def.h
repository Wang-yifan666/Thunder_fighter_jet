#pragma once

#include <chrono>

#include "game/game_config.h"
#include "game/game_session.h"
#include "game/session_snapshot.h"
#include "io/file_score_store.h"

constexpr const char* GAME_NAME = "雷霆战机";
constexpr const char* GAME_VERSION = "v1.4";

/// 终端前端：只负责采集输入，并把 SessionSnapshot 渲染到终端。
/// 菜单 / 难度 / 排行榜 / 游戏 / 结算等流程状态机在 thunder::GameSession。
class ThunderFighter
{
public:
	ThunderFighter();
	~ThunderFighter() = default;

	/// 推进一帧（由 main 的循环调用；本函数不再包含阻塞菜单循环）
	void Run();

	bool IsRunning() const { return running_; }

private:
	void DrawPlaying(const thunder::SessionSnapshot& snapshot);
	void DrawScene(const thunder::SessionSnapshot& snapshot);

	thunder::FileScoreStore score_store_;
	thunder::GameSession session_;

	bool test_mode_;
	bool running_;

	// 场景类界面（菜单/结算）按原节奏重绘，避免每帧刷屏
	bool scene_drawn_ = false;
	thunder::Screen last_scene_ = thunder::Screen::Menu;
	std::chrono::steady_clock::time_point last_scene_draw_;

	// 终端计时与帧率统计
	std::chrono::steady_clock::time_point last_tick_;
	std::chrono::steady_clock::time_point last_fps_time_;
	int fps_counter_ = 0;
	int current_fps_ = 0;
};
