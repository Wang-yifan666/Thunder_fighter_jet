// Naidan 入口：固定逻辑步长，渲染帧率与显示器刷新率解耦。
// 游戏规则全部来自 thunder_logic 的 GameSession，本文件只做事件/绘制/计时。

#include <SDL3/SDL.h>

#include <chrono>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <string>

#include "game/game_config.h"
#include "game/game_session.h"
#include "game/game_info.h"
#include "game/input_state.h"
#include "io/file_score_store.h"
#include "io/score_path.h"
#include "sdl_font.h"
#include "sdl_input.h"
#include "sdl_render.h"

namespace
{

constexpr auto kLogicStep = std::chrono::milliseconds(GameConfig::kFrameDelayMs);
constexpr int kMaxStepsPerFrame = 5; // 卡顿时最多补 5 步，避免死亡螺旋

int ParseIntOption(int argc, char** argv, const char* name)
{
	for(int i = 1; i + 1 < argc; ++i)
	{
		if(std::strcmp(argv[i], name) == 0)
			return std::atoi(argv[i + 1]);
	}
	return 0;
}

bool HasFlag(int argc, char** argv, const char* flag)
{
	for(int i = 1; i < argc; ++i)
	{
		if(std::strcmp(argv[i], flag) == 0)
			return true;
	}
	return false;
}

const char* ParseStringOption(int argc, char** argv, const char* name)
{
	for(int i = 1; i + 1 < argc; ++i)
	{
		if(std::strcmp(argv[i], name) == 0)
			return argv[i + 1];
	}
	return nullptr;
}

/// 把当前帧存成原始 RGBA（"宽 高\n" + 像素），
/// 由 tools/shots_to_png.py 转成 PNG。用于更新文档截图。
bool SaveFrame(SDL_Renderer* renderer, const char* path)
{
	SDL_Surface* shot = SDL_RenderReadPixels(renderer, nullptr);
	if(shot == nullptr)
	{
		std::fprintf(stderr, "screenshot failed: %s\n", SDL_GetError());
		return false;
	}

	SDL_Surface* rgba = SDL_ConvertSurface(shot, SDL_PIXELFORMAT_RGBA32);
	SDL_DestroySurface(shot);
	if(rgba == nullptr)
	{
		std::fprintf(stderr, "convert failed: %s\n", SDL_GetError());
		return false;
	}

	const int width = rgba->w;
	const int height = rgba->h;
	const int pitch = rgba->pitch;

	FILE* fp = std::fopen(path, "wb");
	if(fp == nullptr)
	{
		SDL_DestroySurface(rgba);
		return false;
	}

	std::fprintf(fp, "%d %d\n", width, height);
	const unsigned char* base = static_cast<const unsigned char*>(rgba->pixels);
	for(int y = 0; y < height; ++y)
	{
		std::fwrite(base + static_cast<std::size_t>(y) * pitch, 1,
		            static_cast<std::size_t>(width) * 4, fp);
	}
	std::fclose(fp);
	SDL_DestroySurface(rgba);

	std::printf("screenshot: %s (%dx%d)\n", path, width, height);
	return true;
}

/// 脚本化输入（无头冒烟与截图共用）：
/// 主菜单按 F 开作弊 -> 选难度 -> 开火横移 -> 到固定步数自杀进结算。
thunder::RawInput DemoInput(unsigned int step)
{
	thunder::RawInput raw;
	auto set = [&raw](thunder::Key key) {
		raw.down.set(thunder::KeyIndex(key));
	};

	if(step < 6) // 主菜单：打开作弊模式（否则游戏内数字键无效）
		set(thunder::Key::CheatUnlock);
	else if(step >= 12 && step < 18) // 菜单 -> 难度选择
		set(thunder::Key::Digit1);
	else if(step >= 24 && step < 30) // 选 Easy -> 进入游戏
		set(thunder::Key::Digit1);
	else if(step >= 300 && step < 306) // 作弊自杀 -> 结算界面
		set(thunder::Key::Digit2);
	else if(step >= 36)
	{
		// 点射而不是长按：长按会每步发一颗，串成一条连续的子弹长龙
		if(step % 8 < 3)
			set(thunder::Key::Fire);
		if((step / 20) % 2 == 0)
			set(thunder::Key::Left);
		else
			set(thunder::Key::Right);
	}

	return raw;
}

} // namespace

int main(int argc, char** argv)
{
	if(HasFlag(argc, argv, "--help"))
	{
		std::printf("%s\n"
		            "  --frames N   run N frames then exit (headless smoke)\n"
		            "  --demo       feed scripted input (menu -> play -> game over)\n"
		            "  --no-vsync   disable vertical sync (uncapped rendering)\n"
		            "  --max-fps N  limit render rate (only useful with --no-vsync)\n"
		            "  --seed N     fix the RNG seed (reproducible demo / screenshots)\n"
		            "  --screenshot PATH\n"
		            "               dump the last frame as raw RGBA (for docs)\n"
		            "  --help       show this help\n"
		            "In-game: WASD move, SPACE fire, P pause, Q menu, F cheats,\n"
		            "         F11 fullscreen, ESC quit\n",
		            GameInfo::kProductNameEn);
		return 0;
	}

	const int max_frames = ParseIntOption(argc, argv, "--frames");
	const int max_fps = ParseIntOption(argc, argv, "--max-fps");
	const bool demo = HasFlag(argc, argv, "--demo");
	const bool no_vsync = HasFlag(argc, argv, "--no-vsync");
	const char* shot_path = ParseStringOption(argc, argv, "--screenshot");
	// 0 = 随机播种（正常游玩）；非 0 时场景完全可复现，供截图与冒烟使用
	const auto seed = static_cast<std::uint32_t>(ParseIntOption(argc, argv, "--seed"));

	if(!SDL_Init(SDL_INIT_VIDEO))
	{
		std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
		return 1;
	}

	// 优先使用 Ubuntu Mono；找不到字体或未编译 SDL3_ttf 时自动回退点阵字体
	const bool ttf_font = SdlFont::Init();

	// 窗口标题走系统渲染，可以直接用中文
	const std::string window_title =
	    std::string(GameInfo::kProductNameZh) + " " + GameInfo::kVersionTag;
	SDL_Window* window = SDL_CreateWindow(window_title.c_str(), 832, 496, SDL_WINDOW_RESIZABLE);
	SDL_Renderer* renderer = window != nullptr ? SDL_CreateRenderer(window, nullptr) : nullptr;

	if(renderer != nullptr)
	{
		// 默认开启垂直同步；--no-vsync 用于取消同步（可配合 --max-fps 限帧）
		SDL_SetRenderVSync(renderer, no_vsync ? 0 : 1);
		// 逻辑分辨率固定 832x496；窗口缩放时按比例缩放并留黑边
		SDL_SetRenderLogicalPresentation(renderer, 832, 496, SDL_LOGICAL_PRESENTATION_LETTERBOX);
		SDL_SetWindowMinimumSize(window, 416, 248);
	}
	else
	{
		std::fprintf(stderr, "renderer unavailable (%s)\n", SDL_GetError());
	}

	// 排行榜文件优先放在 exe 同目录（单文件便携运行），兼容旧的上一级路径
	std::string exe_dir;
	if(const char* base = SDL_GetBasePath())
		exe_dir = base;

	thunder::FileScoreStore score_store(
	    thunder::ResolveScorePath(exe_dir.empty() ? ".\\" : exe_dir));
	thunder::GameSession session(score_store, seed);

	SdlInput::State input;
	std::chrono::steady_clock::duration accumulator{};

	Uint64 last_ns = SDL_GetTicksNS();
	Uint64 fps_window_start = last_ns;
	int fps_frames = 0;
	int fps = 0;
	int frames = 0;
	unsigned int demo_step = 0;
	Uint64 next_frame_ns = 0; // --no-vsync 时的软限帧
	bool fullscreen = false;
	bool running = true;

	while(running)
	{
		SDL_Event event;
		while(SDL_PollEvent(&event))
		{
			if(event.type == SDL_EVENT_QUIT)
				running = false;

			if(event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat
			   && event.key.scancode == SDL_SCANCODE_F11 && window != nullptr)
			{
				fullscreen = !fullscreen;
				SDL_SetWindowFullscreen(window, fullscreen);
			}
			input.HandleEvent(event);
		}

		const Uint64 now_ns = SDL_GetTicksNS();

		int steps = 0;

		if(demo)
		{
			// 确定性冒烟：每帧固定推进一个逻辑步，不依赖墙上时间
			session.Step(DemoInput(demo_step++), kLogicStep);
			steps = 1;
			accumulator = std::chrono::steady_clock::duration::zero();
		}
		else
		{
			accumulator += std::chrono::nanoseconds(static_cast<long long>(now_ns - last_ns));

			const thunder::RawInput raw = input.Build();

			while(accumulator >= kLogicStep && steps < kMaxStepsPerFrame)
			{
				thunder::RawInput step_input = raw;
				if(steps > 0)
					step_input.pressed_events.reset();

				session.Step(step_input, kLogicStep);
				accumulator -= kLogicStep;
				++steps;
			}

			if(steps > 0)
				input.ConsumePressedEvents(); // 按下事件只被一个逻辑步消费

			if(steps == kMaxStepsPerFrame)
				accumulator = std::chrono::steady_clock::duration::zero();
		}

		last_ns = now_ns;

		if(session.exit_requested())
			running = false;

		const thunder::SessionSnapshot snapshot = session.Snapshot();

		if(renderer != nullptr)
		{
			SDL_SetRenderDrawColor(renderer, 12, 12, 16, 255);
			SDL_RenderClear(renderer);
			SdlRender::Draw(renderer, snapshot, fps, static_cast<double>(now_ns) / 1.0e9);
			SDL_RenderPresent(renderer);
		}
		else if(max_frames == 0)
		{
			std::fprintf(stderr, "no renderer available; aborting\n");
			break;
		}

		++frames;
		++fps_frames;
		if(now_ns - fps_window_start >= 1000000000ULL)
		{
			fps = fps_frames;
			fps_frames = 0;
			fps_window_start = now_ns;
		}

		if(max_frames > 0 && frames >= max_frames)
		{
			if(shot_path != nullptr && renderer != nullptr)
				SaveFrame(renderer, shot_path);
			running = false;
		}

		if(renderer != nullptr && no_vsync && max_fps > 0)
		{
			const Uint64 frame_ns = 1000000000ULL / static_cast<Uint64>(max_fps);
			const Uint64 after_present = SDL_GetTicksNS();
			if(next_frame_ns != 0 && after_present < next_frame_ns)
				SDL_DelayNS(next_frame_ns - after_present);
			next_frame_ns = SDL_GetTicksNS() + frame_ns;
		}

		if(renderer == nullptr)
			SDL_Delay(1); // 无渲染器时避免空转
	}

	if(max_frames > 0)
	{
		std::printf("smoke ok: frames=%d steps=%u screen=%d font=%s\n", frames, demo ? demo_step : 0,
		            static_cast<int>(session.Snapshot().screen),
		            ttf_font ? "ubuntu-mono" : "bitmap");
	}

	SdlFont::Shutdown();

	if(renderer != nullptr)
		SDL_DestroyRenderer(renderer);
	if(window != nullptr)
		SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
