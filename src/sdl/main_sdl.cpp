// SDL3 前端入口：固定 16ms 逻辑步长，渲染帧率与显示器刷新率解耦。
// 游戏规则全部来自 thunder_logic 的 GameSession，本文件只做事件/绘制/计时。

#include <SDL3/SDL.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "game/game_session.h"
#include "game/input_state.h"
#include "io/file_score_store.h"
#include "sdl_font.h"
#include "sdl_input.h"
#include "sdl_render.h"

namespace
{

constexpr auto kLogicStep = std::chrono::milliseconds(16);
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

/// 无头冒烟用：按固定步数注入脚本化输入（菜单 -> 难度 -> 开火移动）。
thunder::RawInput DemoInput(unsigned int step)
{
	thunder::RawInput raw;
	auto set = [&raw](thunder::Key key) {
		raw.down.set(thunder::KeyIndex(key));
	};

	if(step < 6)
		set(thunder::Key::Digit1);
	else if(step >= 20 && step < 26)
		set(thunder::Key::Digit1);
	else if(step >= 30)
	{
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
		std::printf("Thunder Fighter (SDL3)\n"
		            "  --frames N   run N frames then exit (headless smoke)\n"
		            "  --demo       feed scripted input (menu -> difficulty -> play)\n"
		            "  --no-vsync   disable vertical sync (uncapped rendering)\n"
		            "  --max-fps N  limit render rate (only useful with --no-vsync)\n"
		            "  --help       show this help\n"
		            "In-game: WASD move, SPACE fire, P pause, Q menu, "
		            "F11 fullscreen, ESC quit\n");
		return 0;
	}

	const int max_frames = ParseIntOption(argc, argv, "--frames");
	const int max_fps = ParseIntOption(argc, argv, "--max-fps");
	const bool demo = HasFlag(argc, argv, "--demo");
	const bool no_vsync = HasFlag(argc, argv, "--no-vsync");

	if(!SDL_Init(SDL_INIT_VIDEO))
	{
		std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
		return 1;
	}

	// 优先使用 Ubuntu Mono；找不到字体或未编译 SDL3_ttf 时自动回退点阵字体
	const bool ttf_font = SdlFont::Init();

	SDL_Window* window = SDL_CreateWindow(
	    "Thunder Fighter (SDL3)", 800, 496, SDL_WINDOW_RESIZABLE);
	SDL_Renderer* renderer =
	    window != nullptr ? SDL_CreateRenderer(window, nullptr) : nullptr;

	if(renderer != nullptr)
	{
		// 默认开启垂直同步；--no-vsync 用于取消同步（可配合 --max-fps 限帧）
		SDL_SetRenderVSync(renderer, no_vsync ? 0 : 1);
		// 逻辑分辨率固定 800x496；窗口缩放时按比例缩放并留黑边
		SDL_SetRenderLogicalPresentation(renderer, 800, 496,
		                                 SDL_LOGICAL_PRESENTATION_LETTERBOX);
		SDL_SetWindowMinimumSize(window, 400, 248);
	}
	else
	{
		std::fprintf(stderr, "renderer unavailable (%s)\n", SDL_GetError());
	}

	thunder::FileScoreStore score_store;
	thunder::GameSession session(score_store);

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
			   && event.key.scancode == SDL_SCANCODE_F11
			   && window != nullptr)
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
			accumulator += std::chrono::nanoseconds(
			    static_cast<long long>(now_ns - last_ns));

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
			SdlRender::Draw(renderer, snapshot, fps,
			                static_cast<double>(now_ns) / 1.0e9);
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
			running = false;

		if(renderer != nullptr && no_vsync && max_fps > 0)
		{
			const Uint64 frame_ns =
			    1000000000ULL / static_cast<Uint64>(max_fps);
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
		std::printf("smoke ok: frames=%d steps=%u screen=%d font=%s\n", frames,
		            demo ? demo_step : 0,
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
